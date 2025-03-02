#include "scheduler.h"
#include "util.h"
#include <format>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>

namespace sylar {
    Scheduler::Scheduler(std::size_t num, std::string name) : name_(std::move(name)), num_of_threads_(num) {
        SYLAR_ASSERT(num > 0);
        threads_.reserve(num);
    }

    Scheduler::~Scheduler() {
        if (!stop_source_.stop_requested()) {
            stop();
        }
        if (t_current_scheduler == this) {
            t_current_scheduler = nullptr;
        }
    }

    void Scheduler::start() {
        std::scoped_lock<std::mutex> lock(mutex_);
        if (stop_source_.stop_requested()) {
            return;
        }
        SYLAR_ASSERT(threads_.empty());
        for (std::size_t i = 0; i < num_of_threads_; i++) {
            auto thread = Thread([this]() { run(); }, std::format("{}_{}", name_, i));
            thread.start();
            threads_.emplace_back(std::move(thread));
        }
    }

    void Scheduler::stop() {
        stop_source_.request_stop();
        std::vector<Thread> threads;
        {
            std::scoped_lock<std::mutex> lock(mutex_);
            threads.swap(threads_);
        }
        for (auto& thread : threads) {
            thread.join();
        }
    }

    void Scheduler::run() {
        spdlog::debug("{} run", name_);
        t_current_scheduler = this;
        t_current_scheduler_fiber = Fiber::getCurrentFiber().get();

        Task task{};
        auto stop_token = stop_source_.get_token();

        while (true) {
            Task task{};
            {
                std::unique_lock<std::mutex> lock(mutex_);
                if (!cond_.wait(lock, stop_token, [this]() { return !tasks_.empty(); })) {
                    spdlog::debug("{} stop", name_);
                    return;
                }

                task = std::move(tasks_.front());
                tasks_.pop();
                active_threads_++;
            }
            SYLAR_ASSERT(task);

            auto state = task->getState();
            SYLAR_ASSERT(state == Fiber::INIT || state == Fiber::READY || state == Fiber::HOLD);

            task->swapIn();
            active_threads_--;
            state = task->getState();
            if (state == Fiber::READY || state == Fiber::HOLD) {
                schedule(std::move(task));
            }
        }
    }

    void Scheduler::schedule(Task task) {
        {
            std::scoped_lock<std::mutex> lock(mutex_);
            tasks_.push(std::move(task));
        }
        cond_.notify_all();
    }
    void Scheduler::schedule(Fiber::FiberFunc const& func, std::size_t num) {
        {
            std::scoped_lock<std::mutex> lock(mutex_);
            for (std::size_t i = 0; i < num; i++) {
                tasks_.push(Fiber::newFiber(func));
            }
        }
        cond_.notify_all();
    }

} // namespace sylar
