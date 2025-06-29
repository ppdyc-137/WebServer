#include "fiber.h"
#include "processor.h"
#include "util.h"

#include <cassert>
#include <memory>
#include <spdlog/spdlog.h>
#include <ucontext.h>

namespace sylar {
    using namespace boost::context::detail;

    namespace {
#pragma GCC optimize("O0")
        Fiber* getCurrentMainFiber() { return Processor::getProcessorFiber(); }
    } // namespace

    Fiber::Fiber() : state_(EXEC) { t_current_fiber = this; }

    Fiber* Fiber::newFiber(Func func, uint32_t stack_size) { return new Fiber(std::move(func), stack_size); }

    Fiber::Fiber(Func func, uint32_t stack_size)
        : stack_size_(stack_size), fiber_id_(++next_fiber_id), func_(std::move(func)),
          stack_(std::make_unique<char[]>(stack_size_)),
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
            assertThat(state_ == INIT || state_ == TERM || state_ == EXCEPT);
        } else {
            // main fiber
            checkRet(!func_);
            assertThat(state_ == EXEC);
            if (t_current_fiber == this) {
                t_current_fiber = nullptr;
            }
        }
    }

    void Fiber::swapIn() {
        assertThat(t_current_fiber == getCurrentMainFiber());

        state_ = EXEC;
        t_current_fiber = this;
        context_ = jump_fcontext(context_, nullptr).fctx;
    }

    void Fiber::swapOut(State state) {
        assertThat(state != EXEC);
        assertThat(t_current_fiber == this);

        auto* main_fiber = getCurrentMainFiber();
        t_current_fiber = main_fiber;
        state_ = state;

        // spdlog::debug("{} jump {}:{}", (void*)this, (void*)main_fiber, (void*)main_fiber->context_);
        assertThat(main_fiber->context_ != nullptr);
        auto* from = jump_fcontext(main_fiber->context_, nullptr).fctx;

        // FIXME 当一个协程被窃取后调度到另外一个Processor时，这里获取到的main_fiber还是原来Processor的
        main_fiber = getCurrentMainFiber();
        main_fiber->context_ = from;

        // spdlog::debug("{} back {}:{}", (void*)this, (void*)main_fiber, (void*)main_fiber->context_);
    }

    void Fiber::yield(State state) {
        assertThat(t_current_fiber != nullptr);
        assertThat(t_current_fiber->state_ == EXEC);
        t_current_fiber->swapOut(state);
    }

    void Fiber::run(boost::context::detail::transfer_t arg) {
        getCurrentMainFiber()->context_ = arg.fctx;

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
        t_current_fiber = getCurrentMainFiber();
        t_current_fiber->state_ = EXEC;

        jump_fcontext(t_current_fiber->context_, nullptr);
    }

} // namespace sylar
