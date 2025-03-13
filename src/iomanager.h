#pragma once

#include "mutex.h"
#include "scheduler.h"
#include "timer.h"
#include "util.h"

#include <atomic>
#include <functional>
#include <sys/epoll.h>
#include <vector>

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
        struct FDContext {
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

        bool stopable() { return stop_source_.stop_requested() && num_of_events_ == 0 && !hasTimer(); }

        void resize(std::size_t num);
        bool delEvent(int fd, EPOLL_EVENTS event, bool trigger);

        void tickle() override;
        void timerInsertTickle() override { tickle(); }
        void idle() override;

        int epfd_{};
        int ticklefd_[2]{};
        std::vector<std::shared_ptr<FDContext>> fd_contexts_;
        std::atomic<std::size_t> num_of_events_;
        RWMutex mutex_;
    };

} // namespace sylar
