#pragma once

#include "detail/fiber.h"

#include <functional>
#include <mutex>
#include <queue>

namespace sylar {
    class RunQueue {
    public:
        using Func = std::function<void()>;
        using Task = Fiber*;

        void emplace(Task task) {
            std::lock_guard<std::mutex> lock(mutex_);
            tasks_.push(task);
        }
        void emplace(Func const& func) {
            std::lock_guard<std::mutex> lock(mutex_);
            tasks_.push(buildTask(func));
        }

        // free tasks only be used in local thread, lock isn't needed here
        void emplace_free(Task task) { free_tasks_.push(task); }

        // get a task from the free queue or create a new one
        Task buildTask(Func const& func) {
            Task task = nullptr;
            if (!free_tasks_.empty()) {
                task = free_tasks_.front();
                free_tasks_.pop();
                task->reset(func);
            } else {
                task = Fiber::newFiber(func);
            }
            return task;
        }

        // steal half of tasks
        size_t steal(RunQueue& rq, bool stealRunNext) {
            std::scoped_lock lock(mutex_, rq.mutex_);
            if (tasks_.empty()) {
                return 0;
            }
            if (tasks_.size() == 1 && !stealRunNext) {
                return 0;
            }

            auto len = (tasks_.size() + 1) / 2;
            for (size_t i = 0; i < len; i++) {
                rq.tasks_.push(tasks_.front());
                tasks_.pop();
            }
            return len;
        }

        size_t size() {
            std::lock_guard<std::mutex> lock(mutex_);
            return tasks_.size();
        }

        Task pop() {
            std::lock_guard<std::mutex> lock(mutex_);
            if (tasks_.empty()) {
                return nullptr;
            }
            Task task = tasks_.front();
            tasks_.pop();
            return task;
        }

    private:
        std::mutex mutex_;
        std::queue<Task> tasks_;
        std::queue<Task> free_tasks_;
    };

} // namespace sylar
