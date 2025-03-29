#pragma once

#include "mutex.h"
#include "scheduler.h"
#include "timer.h"
#include "util.h"

#include <atomic>
#include <functional>
#include <spdlog/spdlog.h>
#include <sys/epoll.h>

namespace sylar {
    class IOManager : public Scheduler, public TimerManager {
    public:
        using EventCallBackFunc = std::function<void()>;

        explicit IOManager(std::size_t num, std::string name = "UNKNOWN");
        ~IOManager() override;

        void stop() override;

        bool addEvent(int fd, EPOLL_EVENTS event, EventCallBackFunc func = nullptr);
        bool delEvent(int fd, EPOLL_EVENTS event) { return delEvent(fd, event, false); }
        bool cancelEvent(int fd, EPOLL_EVENTS event) { return delEvent(fd, event, true); }
        bool cancelAll(int fd);

        static IOManager* getCurrentScheduler() { return dynamic_cast<IOManager*>(t_current_scheduler); }

    private:
        struct FDEventContext {
            struct EventContext {
                Scheduler* scheduler_;
                std::shared_ptr<Fiber> fiber_;
                EventCallBackFunc func_;
            };

            int fd_;
            uint32_t events_{};
            EventContext read_;
            EventContext write_;
            Mutex mutex_;

            EventContext& getContext(uint32_t event) {
                if (event == EPOLLIN) {
                    return read_;
                }
                if (event == EPOLLOUT) {
                    return write_;
                }
                SYLAR_ASSERT(false);
            }

            void resetContext(uint32_t event) {
                auto& context = getContext(event);
                context.scheduler_ = nullptr;
                context.fiber_.reset();
                context.func_ = nullptr;
            }

            void triggerContext(uint32_t event) {
                auto& context = getContext(event);
                if (context.fiber_) {
                    context.scheduler_->schedule(context.fiber_);
                    context.fiber_ = nullptr;
                }
                if (context.func_) {
                    context.scheduler_->schedule(context.func_);
                    context.func_ = nullptr;
                }
                context.scheduler_ = nullptr;
            }
        };

        bool stopable() {
            auto res =  num_of_events_ == 0 && !hasTimer() && numOfTasks() == 0 && active_threads_ == 0 && stop_source_.stop_requested();
            return res;
            // if (num_of_events_ != 0) {
            //     spdlog::debug("cannot not stop because num_of_events_({}) != 0", num_of_events_.load());
            //     return false;
            // }
            // if (hasTimer()) {
            //     spdlog::debug("cannot not stop because has timer");
            //     return false;
            // }
            // if (numOfTasks() != 0) {
            //     spdlog::debug("cannot not stop because numOfTasks({}) != 0", numOfTasks());
            //     return false;
            // }
            // if (active_threads_ != 0) {
            //     spdlog::debug("cannot not stop because active_threads({}, {}) != 0", active_threads_.load(), idle_threads_.load());
            //     return false;
            // }
            // if (!stop_source_.stop_requested()) {
            //     spdlog::debug("cannot not stop because !stop_source.stop_requested()");
            //     return false;
            // }
            // return true;
        }

        bool delEvent(int fd, EPOLL_EVENTS event, bool trigger);
        FDEventContext* getFDEventContext(int fd) {
            if (!fd_contexts_.contains(fd)) {
                fd_contexts_[fd].fd_ = fd;
            }
            return &fd_contexts_[fd];
        }

        void tickle() override;
        void timerInsertTickle() override { tickle(); }
        void idle() override;

        int epfd_{};
        int ticklefd_[2]{};
        std::unordered_map<int , FDEventContext> fd_contexts_;
        std::atomic<std::size_t> num_of_events_;
        Mutex mutex_;
    };

} // namespace sylar
