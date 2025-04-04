#pragma once

#include "fiber.h"
#include "timer.h"

#include <cstdint>
#include <liburing.h>
#include <optional>
#include <queue>
#include <spdlog/spdlog.h>

static constexpr unsigned int RING_SIZE = 256;
namespace sylar {
    class IOContext : public TimerManager {
    public:
        using Func = std::function<void()>;
        using Task = Fiber*;

        explicit IOContext(bool hook = false, unsigned int entries = RING_SIZE);
        ~IOContext();

        void execute();
        bool runOnce();

        void schedule(Func const& func) { schedule(getTask(func)); }
        void schedule(Task);

        uint64_t getPendingOps() const { return pending_ops_; }

        static IOContext* getCurrentContext() { return t_context; }
        static Fiber* getCurrentContextFiber() { return &t_context_fiber; }

        static void spawn(Func const& func);
        static void spawn(Task);

    private:
        Task getTask(Func const&);

        void runTask(Func const& func) { runTask(getTask(func)); }
        void runTask(Task);

        void waitEvent(std::optional<std::chrono::system_clock::duration>);

        friend struct UringOp;
        struct io_uring_sqe* getSqe();

        io_uring uring_{};

        std::atomic<uint64_t> pending_ops_;

        std::queue<Task> ready_tasks_;
        std::queue<Task> free_tasks_;

        static inline thread_local IOContext* t_context{};
        static inline thread_local Fiber t_context_fiber{};
    };

} // namespace sylar
