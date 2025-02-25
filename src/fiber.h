#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <sys/ucontext.h>
#include <ucontext.h>

namespace sylar {

    class Fiber : public std::enable_shared_from_this<Fiber> {
    public:
        using fiber_function = std::function<void()>;
        enum State : uint8_t {
            INIT,
            READY,
            HOLD,
            EXEC,
            TERM,
            EXCEPT,
        };

        constexpr static uint32_t DEFAULT_STACK_SIZE = 4 * 1024 * 1024;
        explicit Fiber(fiber_function func, uint32_t stack_size = DEFAULT_STACK_SIZE);
        ~Fiber();

        void swapIn();
        void swapOut();

        uint64_t getId() const { return id_; }
        State getState() const { return state_; }

        static std::shared_ptr<Fiber> getCurrentFiber();
        static uint64_t getCurrentFiberId();

    private:
        // TODO initialize main fiber in new thread
        Fiber(); // main fiber in new thread

        static void run();

        uint64_t id_;
        State state_{INIT};
        uint32_t stack_size_{};

        fiber_function func_;
        std::unique_ptr<char[]> stack_;
        ucontext_t context_{};
    };

} // namespace sylar
