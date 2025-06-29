#pragma once

#include "detail/fiber.h"
#include "detail/timer.h"
#include "runqueue.h"

#include <cstdint>
#include <liburing.h>
#include <spdlog/spdlog.h>

static constexpr unsigned int RING_SIZE = 256;
static constexpr size_t RUNQUEUE_SIZE = 256;
namespace sylar {
    class Processor : public TimerManager {
    public:
        using Func = std::function<void()>;
        using Task = Fiber*;

        explicit Processor(uint64_t id, bool hook = false, unsigned int entries = RING_SIZE);
        ~Processor();

        void execute();

        // push task into local runqueue, if it's full, push into global runqueue
        void emplaceTask(Func const& func) { emplaceTask(rq_.buildTask(func)); }
        void emplaceTask(Task task);
        size_t stealTasks(RunQueue& rq) { return rq_.steal(rq, false); }

        uint64_t getPendingOps() const { return pending_ops_; }

        // get current thread's processor
        static Processor* getProcessor() { return t_processor; }
        static uint64_t getProcessorID() { return t_processor->id_; }
        static Fiber* getProcessorFiber() { return &t_processor->processor_fiber_; }
        Fiber* getFiber() { return &processor_fiber_; }
        uint64_t getId() const { return id_; }

    private:
        bool execOnce();

        void execTask(Func const& func) { execTask(rq_.buildTask(func)); }
        void execTask(Task);

        void waitEvent(std::chrono::system_clock::duration);

        friend struct UringOp;
        struct io_uring_sqe* getSqe();

        uint64_t id_;

        io_uring uring_{};

        std::atomic<uint64_t> pending_ops_;

        RunQueue rq_{RUNQUEUE_SIZE};

        Fiber processor_fiber_;

        static inline thread_local Processor* t_processor{};
    };

} // namespace sylar
