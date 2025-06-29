#pragma once

#include "detail/fiber.h"

#include <atomic>
#include <functional>

namespace async {
    class RunQueue {
    public:
        explicit RunQueue(size_t cap) : cap_(cap), top_(0), bottom_(0), tasks_(cap) {}
        RunQueue(const RunQueue&) = delete;

        using Func = std::function<void()>;
        using Task = Fiber*;

        bool try_push(Task task) {
            auto b = bottom_.load(std::memory_order::relaxed);
            auto t = top_.load(std::memory_order::acquire);
            if (b - t >= cap_) {
                return false;
            }
            tasks_[b % cap_] = task;
            bottom_.store(b + 1, std::memory_order::release);
            return true;
        }
        // free tasks only be used in local thread, lock isn't needed here
        void push_free(Task task) {
            task->next_ = free_list_;
            free_list_ = task;
        }

        // get a task from the free queue or create a new one
        Task buildTask(Func const& func) {
            Task task = nullptr;
            if (free_list_ != nullptr) {
                task = free_list_;
                free_list_ = free_list_->next_;
                task->reset(func);
            } else {
                task = Fiber::newFiber(func);
            }
            task = Fiber::newFiber(func);
            return task;
        }

        // steal half of tasks
        size_t steal(RunQueue& rq, bool stealRunNext) {
            if (was_empty()) {
                return 0;
            }
            if (was_size() == 1 && !stealRunNext) {
                return 0;
            }

            auto len = std::min((was_size() + 1) / 2, rq.cap_);
            size_t cnt{};
            for (size_t i = 0; i < len; i++) {
                Task task = try_steal();
                if (!task) {
                    break;
                }

                rq.try_push(task);
                cnt++;
            }
            return cnt;
        }

        bool was_empty() { return was_size() == 0; }
        size_t was_size() {
            auto b = bottom_.load(std::memory_order::relaxed);
            auto t = top_.load(std::memory_order::acquire);
            return (b <= t ? 0 : (b - t));
        }

        Task try_pop() {
            auto b = bottom_.load(std::memory_order::relaxed);
            auto t = top_.load(std::memory_order::acquire);
            if (t >= b) {
                return nullptr;
            }
            size_t newb = b - 1;
            bottom_.store(newb, std::memory_order::relaxed);
            std::atomic_thread_fence(std::memory_order::seq_cst);
            t = top_.load(std::memory_order::relaxed);
            if (t > newb) {
                bottom_.store(b, std::memory_order::relaxed);
                return nullptr;
            }
            Task task = tasks_[newb % cap_];
            if (t != newb) {
                return task;
            }
            auto popped =
                top_.compare_exchange_strong(t, t + 1, std::memory_order::seq_cst, std::memory_order::relaxed);
            bottom_.store(b, std::memory_order::relaxed);
            return popped ? task : nullptr;
        }

        Task try_steal() {
            auto b = bottom_.load(std::memory_order::relaxed);
            auto t = top_.load(std::memory_order::acquire);
            if (t >= b) {
                return nullptr;
            }
            Task task{};
            do {
                std::atomic_thread_fence(std::memory_order::seq_cst);
                b = bottom_.load(std::memory_order::acquire);
                if (t >= b) {
                    return nullptr;
                }
                task = tasks_[t % cap_];
            } while (!top_.compare_exchange_strong(t, t + 1, std::memory_order::seq_cst, std::memory_order::relaxed));
            return task;
        }

    private:
        size_t cap_;
        std::atomic<size_t> top_;
        std::atomic<size_t> bottom_;
        std::vector<Task> tasks_;
        Task free_list_{};
    };
} // namespace async
