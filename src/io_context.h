#pragma once

#include "fiber.h"

#include <cstdint>
#include <liburing.h>
#include <queue>
#include <spdlog/spdlog.h>

namespace sylar {
    class IOContext {
    public:
        using Func = std::function<void()>;
        using Task = Fiber*;

        IOContext();
        ~IOContext();

        void execute();
        bool runOnce();

        void schedule(Func const& func);

        static IOContext* getCurrentContext() { return t_context; }
        static Fiber* getCurrentContextFiber() { return &t_context_fiber; }
        uint64_t getPendingOps() const { return pending_ops_; }

    private:
        void schedule(Task);

        friend struct UringOp;
        struct io_uring_sqe* getSqe();

        static constexpr int RING_SIZE = 256;
        io_uring uring_{};

        std::atomic<uint64_t> pending_ops_;

        std::queue<Task> ready_tasks_;
        std::queue<Task> free_tasks_;

        static inline thread_local IOContext* t_context{};
        static inline thread_local Fiber t_context_fiber{};
    };

} // namespace sylar
