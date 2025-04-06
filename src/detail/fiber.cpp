#include "fiber.h"
#include "io_context.h"
#include "util.h"

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <memory>
#include <spdlog/spdlog.h>
#include <ucontext.h>

namespace sylar {
    using namespace boost::context::detail;

    Fiber::Fiber() : state_(EXEC) { t_current_fiber = this; }

    Fiber* Fiber::newFiber(Func func, uint32_t stack_size) { return new Fiber(std::move(func), stack_size); }

    Fiber::Fiber(Func func, uint32_t stack_size)
        : stack_size_(stack_size), func_(std::move(func)), stack_(std::make_unique<char[]>(stack_size_)),
          context_(make_fcontext(stack_.get() + stack_size_, stack_size_, &Fiber::run)) {

        if (func_) {
            state_ = READY;
        }
    }

    void Fiber::reset(Func func) {
        state_ = READY;
        func_ = std::move(func);

        context_ = make_fcontext(stack_.get() + stack_size_, stack_size_, &Fiber::run);
    }

    Fiber::~Fiber() {
        if (stack_) {
            myAssert(state_ == INIT || state_ == TERM || state_ == EXCEPT);
        } else {
            // main fiber
            checkRet(!func_);
            myAssert(state_ == EXEC);
            if (t_current_fiber == this) {
                t_current_fiber = nullptr;
            }
        }
    }

    void Fiber::swapIn() {
        myAssert(t_current_fiber == IOContext::getCurrentContextFiber());

        state_ = EXEC;
        t_current_fiber = this;
        context_ = jump_fcontext(context_, nullptr).fctx;
    }

        myAssert(t_current_fiber == this);
    void Fiber::swapOut(State state) {

        // a fiber can only swap out to the scheduler fiber
        t_current_fiber = IOContext::getCurrentContextFiber();
        myAssert(t_current_fiber);

        myAssert(state_ != EXEC);
        state_ = state;
        IOContext::getCurrentContextFiber()->context_ = jump_fcontext(t_current_fiber->context_, nullptr).fctx;
    }

        myAssert(t_current_fiber != nullptr);
        myAssert(t_current_fiber->state_ == EXEC);
    void Fiber::yield(State state) {
        t_current_fiber->swapOut(state);
    }

    void Fiber::run(boost::context::detail::transfer_t arg) {
        IOContext::getCurrentContextFiber()->context_ = arg.fctx;

        {
            auto* fiber = getCurrentFiber();
            try {
                fiber->func_();
                fiber->func_ = nullptr;
                fiber->state_ = TERM;
            } catch (std::exception& ex) {
                fiber->state_ = EXCEPT;
                spdlog::error("Fiber::run error: {}", ex.what());
            } catch (...) {
                fiber->state_ = EXCEPT;
                spdlog::error("Fiber::run error");
            }
        }
        t_current_fiber = IOContext::getCurrentContextFiber();
        t_current_fiber->state_ = EXEC;

        jump_fcontext(t_current_fiber->context_, nullptr);
    }

} // namespace sylar
