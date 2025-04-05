#pragma once

#include <boost/context/detail/fcontext.hpp>
#include <functional>
#include <memory>

namespace sylar {

    class Fiber {
    public:
        using Func = std::function<void()>;
        enum State : uint8_t {
            INIT,
            READY,
            EXEC,
            TERM,
            EXCEPT,
        };
        static constexpr const char* STATE_STR[] = {"INIT", "READY", "EXEC", "TERM", "EXCEPT"};

        constexpr static uint32_t DEFAULT_STACK_SIZE = 1 * 1024 * 1024;
        static Fiber* newFiber(Func func = nullptr, uint32_t stack_size = DEFAULT_STACK_SIZE);
        ~Fiber();

        void resume() { swapIn(); }
        static void yield();

        static Fiber* getCurrentFiber() { return t_current_fiber; }

        const char* getStateStr() { return STATE_STR[static_cast<std::size_t>(state_)]; };
        State getState() { return state_; }

    private:
        friend class IOContext;
        Fiber(Func func, uint32_t stack_size);
        Fiber(); // main fiber in new thread

        static void run(boost::context::detail::transfer_t t);
        void swapIn();
        void swapOut();
        void reset(Func func);

        State state_{INIT};
        uint32_t stack_size_{};

        Func func_;
        std::unique_ptr<char[]> stack_;

        boost::context::detail::fcontext_t context_{};

        static inline thread_local Fiber* t_current_fiber{};
    };

} // namespace sylar
