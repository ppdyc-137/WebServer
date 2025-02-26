#include "fiber.h"
#include "util.h"
#include <cerrno>
#include <cstdlib>
#include <memory>
#include <spdlog/spdlog.h>
#include <ucontext.h>

namespace sylar {
    constexpr size_t STACK_ALIGNMENT = 8;

    Fiber::Fiber() : id_(++g_fiber_id), state_(EXEC) {
        t_current_fiber = this;

        SYLAR_ASSERT2(getcontext(&context_) != -1, std::strerror(errno));
        ++g_fiber_count;
        spdlog::debug("Fiber::Fiber: main fiber, id:{}", id_);
    }

    std::shared_ptr<Fiber> Fiber::newMainFiber() { return std::shared_ptr<Fiber>{new Fiber()}; }

    Fiber::Fiber(fiber_function func, uint32_t stack_size)
        : id_(++g_fiber_id), stack_size_(stack_size), func_(std::move(func)),
          stack_(std::make_unique<char[]>(stack_size_)) {

        SYLAR_ASSERT2(getcontext(&context_) != -1, std::strerror(errno));

        context_.uc_stack.ss_sp = stack_.get();
        context_.uc_stack.ss_size = stack_size_;
        context_.uc_link = nullptr;
        makecontext(&context_, &Fiber::run, 0);

        ++g_fiber_count;
        spdlog::debug("Fiber::Fiber: new fiber, id:{}", id_);
    }

    Fiber::~Fiber() {
        --g_fiber_count;
        spdlog::debug("Fiber::~Fiber: id:{}", id_);

        if (stack_) {
            SYLAR_ASSERT(state_ == INIT || state_ == TERM || state_ == EXCEPT);
        } else {
            // main fiber
            SYLAR_ASSERT(!func_);
            SYLAR_ASSERT(state_ == EXEC);
            if (t_current_fiber == this) {
                t_current_fiber = nullptr;
            }
        }
    }

    std::shared_ptr<Fiber> Fiber::getCurrentFiber() {
        if (t_current_fiber != nullptr) {
            return t_current_fiber->shared_from_this();
        }
        t_main_fiber = newMainFiber();
        SYLAR_ASSERT(t_current_fiber == t_main_fiber.get());
        return t_main_fiber;
    }

    uint64_t Fiber::getCurrentFiberId() {
        if (t_current_fiber != nullptr) {
            return t_current_fiber->id_;
        }
        return 0;
    }

    void Fiber::swapIn() {
        SYLAR_ASSERT(t_current_fiber != nullptr);
        t_current_fiber->state_ = HOLD;
        state_ = EXEC;
        auto* cur = t_current_fiber;
        t_current_fiber = this;
        SYLAR_ASSERT2(swapcontext(&cur->context_, &context_) != -1, std::strerror(errno));
    }

    void Fiber::swapOut() {
        t_current_fiber = t_main_fiber.get();
        t_main_fiber->state_ = EXEC;
        state_ = HOLD;
        SYLAR_ASSERT2(swapcontext(&context_, &t_main_fiber->context_) != -1, std::strerror(errno));
    }

    void Fiber::run() {
        auto fiber = getCurrentFiber();
        try {
            fiber->func_();
            fiber->func_ = nullptr;
            fiber->state_ = TERM;
        } catch (std::exception& ex) {
            fiber->state_ = EXCEPT;
            spdlog::error("Fiber::run: id:{}, error: {}", fiber->id_, ex.what());
        } catch (...) {
            fiber->state_ = EXCEPT;
            spdlog::error("Fiber::run: id:{}, error", fiber->id_);
        }
        t_current_fiber = t_main_fiber.get();
        t_main_fiber->state_ = EXEC;
        fiber.reset();
        SYLAR_ASSERT2(setcontext(&t_main_fiber->context_) != -1, std::strerror(errno));
        SYLAR_ASSERT2(false, std::format("fiber id: {} should not reach here", fiber->id_));
    }

} // namespace sylar
