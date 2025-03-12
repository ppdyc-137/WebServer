#include "scheduler.h"
#include "hook.h"
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
            auto thread = Thread([this, i]() { run(i); }, std::format("{}_{}", name_, i));
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

    void Scheduler::run(std::size_t id) {
        // spdlog::debug("{}_{} run", name_, id);
        t_current_scheduler = this;
        t_current_scheduler_fiber = Fiber::getCurrentFiber().get();
        setHookEnable(true);

        Task task{};
        auto idle_fiber = Fiber::newFiber([this]() { idle(); });

        while (true) {
            Task task{};
            {
                std::unique_lock<std::mutex> lock(mutex_);
                if (!tasks_.empty()) {
                    task = std::move(tasks_.front());
                    tasks_.pop();
                    active_threads_++;
                }
            }
            if (task) {
                spdlog::debug("{}_{} run FiberID: {}", name_, id, task->getId());
                auto state = task->getState();
                if (state == Fiber::EXEC) {
                    continue;
                }
                SYLAR_ASSERT(state == Fiber::INIT || state == Fiber::READY || state == Fiber::HOLD);

                task->swapIn();
                active_threads_--;
                state = task->getState();
                if (state == Fiber::READY) {
                    schedule(std::move(task), false);
                }
            } else {
                // spdlog::debug("{}_{} idle", name_, id);
                idle_threads_++;
                idle_fiber->swapIn();
                idle_threads_--;
                if (idle_fiber->getState() == Fiber::TERM) {
                    spdlog::debug("{}_{} stop", name_, id);
                    return;
                }
                // spdlog::debug("{}_{} resume", name_, id);
            }
        }
    }

    void Scheduler::schedule(Task task, bool tick) {
        // spdlog::debug("schedule FiberID: {}", task->getId());
        {
            std::scoped_lock<std::mutex> lock(mutex_);
            tasks_.push(std::move(task));
        }
        if (tick) {
            tickle();
        }
    }

    void Scheduler::schedule(Task task) {
        // spdlog::debug("schedule FiberID: {}", task->getId());
        bool need_tickle = false;
        {
            std::scoped_lock<std::mutex> lock(mutex_);
            need_tickle = tasks_.empty();
            tasks_.push(std::move(task));
        }
        if (need_tickle) {
            tickle();
        }
    }
    void Scheduler::schedule(Fiber::FiberFunc const& func, std::size_t num) {
        bool need_tickle = false;
        {
            std::scoped_lock<std::mutex> lock(mutex_);
            need_tickle = tasks_.empty();
            for (std::size_t i = 0; i < num; i++) {
                auto task = Fiber::newFiber(func);
                // spdlog::debug("schedule FiberID: {}", task->getId());
                tasks_.push(std::move(task));
            }
        }
        if (need_tickle) {
            tickle();
        }
    }

    void Scheduler::idle() {
        auto stop_token = stop_source_.get_token();
        while (true) {
            {
                std::unique_lock<std::mutex> lock(mutex_);
                if (!cond_.wait(lock, stop_token, [this]() { return !tasks_.empty(); })) {
                    return;
                }
            }
            Fiber::yield();
        }
    }

    void Scheduler::tickle() { cond_.notify_all(); }

} // namespace sylar
