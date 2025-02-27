#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <sys/ucontext.h>
#include <ucontext.h>

namespace sylar {

    class Fiber : public std::enable_shared_from_this<Fiber> {
    public:
        using FiberFunc = std::function<void()>;
        enum State : uint8_t {
            INIT,
            READY,
            HOLD,
            EXEC,
            TERM,
            EXCEPT,
        };

        constexpr static uint32_t DEFAULT_STACK_SIZE = 4 * 1024 * 1024;
        explicit Fiber(FiberFunc func, uint32_t stack_size = DEFAULT_STACK_SIZE);
        ~Fiber();

        void swapIn();
        void swapOut(State state = HOLD);
        static void yield(State state = HOLD);

        uint64_t getId() const { return id_; }
        State getState() const { return state_; }

        static std::shared_ptr<Fiber> getCurrentFiber();
        static uint64_t getCurrentFiberId();

    private:
        Fiber(); // main fiber in new thread
        static std::shared_ptr<Fiber> newMainFiber();

        static void run();

        uint64_t id_;
        State state_{INIT};
        uint32_t stack_size_{};

        FiberFunc func_;
        std::unique_ptr<char[]> stack_;
        ucontext_t context_{};

        static inline std::atomic<uint64_t> g_fiber_id{};
        static inline std::atomic<uint64_t> g_fiber_count{};
        static inline thread_local std::shared_ptr<Fiber> t_main_fiber{newMainFiber()};
        static inline thread_local Fiber* t_current_fiber{};
    };

} // namespace sylar
