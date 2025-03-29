#pragma once

#include "fiber.h"
#include "thread.h"
#include <condition_variable>
#include <mutex>
#include <queue>
#include <vector>

namespace sylar {

    class Scheduler {
    public:
        explicit Scheduler(std::size_t num, std::string name = "UNKNOWN");
        virtual ~Scheduler();

        void start();
        virtual void stop();

        using Task = std::shared_ptr<Fiber>;
        void schedule(Task task);
        void schedule(Fiber::FiberFunc const& func, std::size_t num = 1);

        static Scheduler* getCurrentScheduler() { return t_current_scheduler; }
        static Fiber* getCurrentSchedulerFiber() { return t_current_scheduler_fiber; }

        size_t numOfTasks() {
            std::scoped_lock<std::mutex> lock(mutex_);
            return tasks_.size();
        }

    private:
        void run(std::size_t id);
        virtual void idle();
        virtual void tickle();
        void schedule(Task task, bool tick);

        std::mutex mutex_;
        std::condition_variable_any cond_;

        std::vector<Thread> threads_;
        std::queue<Task> tasks_;
        std::queue<Task> free_tasks_;

    protected:
        std::stop_source stop_source_;

        std::string name_;
        std::size_t num_of_threads_;

        std::atomic<size_t> active_threads_;
        std::atomic<size_t> idle_threads_;

        static inline thread_local Scheduler* t_current_scheduler{};
        static inline thread_local Fiber* t_current_scheduler_fiber{};
    };

} // namespace sylar
