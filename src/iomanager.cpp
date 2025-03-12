#include "iomanager.h"
#include "mutex.h"
#include "scheduler.h"
#include "util.h"
#include <algorithm>
#include <fcntl.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <sys/epoll.h>
#include <unistd.h>

namespace sylar {
    constexpr std::size_t DEFAULT_CONTEXT_SIZE = 32;
    IOManager::IOManager(std::size_t num, std::string name) : Scheduler(num, std::move(name)), epfd_(epoll_create(1)) {
        SYLAR_ASSERT2(epfd_ >= 0, std::strerror(errno));

        auto ret = pipe(ticklefd_);
        SYLAR_ASSERT2(ret >= 0, std::strerror(errno));

        epoll_event event{};
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = ticklefd_[0];

        ret = fcntl(ticklefd_[0], F_SETFL, O_NONBLOCK);
        SYLAR_ASSERT2(ret >= 0, std::strerror(errno));

        ret = epoll_ctl(epfd_, EPOLL_CTL_ADD, ticklefd_[0], &event);
        SYLAR_ASSERT2(ret >= 0, std::strerror(errno));

        resize(DEFAULT_CONTEXT_SIZE);
    }

    IOManager::~IOManager() {
        stop();
        close(epfd_);
        close(ticklefd_[0]);
        close(ticklefd_[1]);
    }

    void IOManager::stop() {
        stop_source_.request_stop();
        for (std::size_t i = 0; i < num_of_threads_; i++) {
            tickle();
        }
        Scheduler::stop();
    }

    bool IOManager::addEvent(int fd, EPOLL_EVENTS event, EventCallBackFunc func) {
        SYLAR_ASSERT(event == EPOLLIN || event == EPOLLOUT);
        std::shared_ptr<FDContext> fd_ctx{};
        {
            mutex_.readLock();
            if (fd_contexts_.size() > static_cast<std::size_t>(fd)) {
                mutex_.unlock();
                fd_ctx = fd_contexts_[static_cast<std::size_t>(fd)];
            } else {
                mutex_.unlock();
                WriteLockGuard lock(mutex_);
                resize(static_cast<std::size_t>(fd) * 2);
                fd_ctx = fd_contexts_[static_cast<std::size_t>(fd)];
            }
        }
        LockGuard lock(fd_ctx->mutex_);

        SYLAR_ASSERT(!(fd_ctx->events_ & event));

        auto op = (fd_ctx->events_ != 0) ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        epoll_event ep_event{};
        ep_event.events = event | fd_ctx->events_ | EPOLLET;
        ep_event.data.ptr = fd_ctx.get();

        auto ret = epoll_ctl(epfd_, op, fd, &ep_event);
        if (ret < 0) {
            spdlog::error("IOManager::addEvent: epoll_ctl {}", strerror(errno));
            return false;
        }
        fd_ctx->events_ = fd_ctx->events_ | event;

        auto& event_context = fd_ctx->getContext(event);
        SYLAR_ASSERT(!event_context.scheduler_ && !event_context.fiber_ && !event_context.func_);
        event_context.scheduler_ = this;
        if (func) {
            event_context.func_ = std::move(func);
        } else {
            event_context.fiber_ = Fiber::getCurrentFiber();
        }

        num_of_events_++;
        return true;
    }

    bool IOManager::delEvent(int fd, EPOLL_EVENTS event, bool trigger) {
        SYLAR_ASSERT(event == EPOLLIN || event == EPOLLOUT);
        std::shared_ptr<FDContext> fd_ctx{};
        {
            mutex_.readLock();
            if (fd_contexts_.size() < static_cast<std::size_t>(fd)) {
                mutex_.unlock();
                return false;
            }
            fd_ctx = fd_contexts_[static_cast<std::size_t>(fd)];
            mutex_.unlock();
        }
        LockGuard lock(fd_ctx->mutex_);

        if ((fd_ctx->events_ & event) == 0) {
            return false;
        }

        auto new_event = fd_ctx->events_ & ~event;
        auto op = (new_event != 0) ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event ep_event{};
        ep_event.events = new_event | EPOLLET;
        ep_event.data.ptr = fd_ctx.get();
        auto ret = epoll_ctl(epfd_, op, fd, &ep_event);
        if (ret < 0) {
            spdlog::error("IOManager::delEvent: epoll_ctl {}", strerror(errno));
            return false;
        }
        fd_ctx->events_ = new_event;

        if (trigger) {
            fd_ctx->triggerContext(event);
        } else {
            fd_ctx->resetContext(event);
        }

        num_of_events_--;
        return true;
    }

    void IOManager::resize(std::size_t num) {
        fd_contexts_.resize(num);

        for (std::size_t i = 0; i < fd_contexts_.size(); i++) {
            if (fd_contexts_[i] == nullptr) {
                fd_contexts_[i] = std::make_shared<FDContext>();
                fd_contexts_[i]->fd_ = static_cast<int>(i);
            }
        }
    }

    void IOManager::tickle() {
        if (idle_threads_ == 0) {
            return;
        }
        auto ret = write(ticklefd_[1], "T", 1);
        // spdlog::debug("tickle");
        SYLAR_ASSERT2(ret >= 0, std::strerror(errno));
    }

    constexpr int MAX_EVENTS = 256;
    constexpr int MAX_TIMEOUT = 3000;
    void IOManager::idle() {
        std::vector<epoll_event> ep_events(MAX_EVENTS);
        while (!stopable()) {
            int timeout = std::min(static_cast<int>(getNextTriggerTimeLast()), MAX_TIMEOUT);
            int num = epoll_wait(epfd_, ep_events.data(), MAX_EVENTS, timeout);
            if (num < 0 && errno == EINTR) {
                continue;
            }

            auto cbs = getExpiredCallBacks();
            for (auto& cb : cbs) {
                schedule(cb);
            }

            for (std::size_t i = 0; i < static_cast<std::size_t>(num); i++) {
                auto& ep_event = ep_events[i];
                if (ep_event.data.fd == ticklefd_[0]) {
                    // spdlog::debug("recv tickle");
                    char buf[1];
                    while (read(ticklefd_[0], buf, sizeof(buf)) > 0) {
                    }
                    continue;
                }
                auto* fd_ctx = static_cast<FDContext*>(ep_event.data.ptr);
                LockGuard lock(fd_ctx->mutex_);
                spdlog::debug("epoll_wait: fd: {}, event: {}", fd_ctx->fd_, strevent(ep_event.events));

                if (ep_event.events & (EPOLLERR | EPOLLHUP)) {
                    ep_event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events_;
                }

                uint32_t event{};
                if (ep_event.events & EPOLLIN) {
                    event |= EPOLLIN;
                }
                if (ep_event.events & EPOLLOUT) {
                    event |= EPOLLOUT;
                }

                if (!(fd_ctx->events_ & event)) {
                    continue;
                }

                auto left_event = fd_ctx->events_ & ~event;
                auto op = left_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
                ep_event.events = left_event | EPOLLET;
                auto ret = epoll_ctl(epfd_, op, fd_ctx->fd_, &ep_event);
                if (ret < 0) {
                    spdlog::error("IOManager::delEvent: epoll_ctl {}", strerror(errno));
                    continue;
                }
                fd_ctx->events_ = left_event;

                if (event & EPOLLIN) {
                    fd_ctx->triggerContext(EPOLLIN);
                    num_of_events_--;
                }
                if (event & EPOLLOUT) {
                    fd_ctx->triggerContext(EPOLLOUT);
                    num_of_events_--;
                }
            }

            Fiber::yield();
        }
    }

} // namespace sylar
