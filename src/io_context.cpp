#include "io_context.h"
#include "detail/hook.h"
#include "uring_op.h"
#include "util.h"

#include <chrono>
#include <liburing.h>
#include <spdlog/spdlog.h>

namespace sylar {
    IOContext::IOContext(bool hook, unsigned int entries) {
        assertThat(t_context == nullptr);
        t_context = this;
        checkRetUring(io_uring_queue_init(entries, &uring_, 0));
        Fiber::t_current_fiber = &t_context_fiber;

        if (hook) {
            setHookEnable(true);
        }
    }

    IOContext::~IOContext() { io_uring_queue_exit(&uring_); }

    struct io_uring_sqe* IOContext::getSqe() {
        struct io_uring_sqe* sqe = io_uring_get_sqe(&uring_);
        while (!sqe) {
            int res = io_uring_submit(&uring_);
            if (res < 0) [[unlikely]] {
                if (res == -EINTR) {
                    continue;
                }
                throw std::system_error(-res, std::system_category());
            }
            sqe = io_uring_get_sqe(&uring_);
        }
        ++pending_ops_;
        return sqe;
    }

    void IOContext::execute() {
        while (runOnce()) {
        }
    }

    bool IOContext::runOnce() {
        while (!ready_tasks_.empty()) {
            Task task = ready_tasks_.front();
            ready_tasks_.pop();
            runTask(task);
        }

        auto timeout = getNextTriggerDuration();
        if (pending_ops_ == 0 && !timeout) {
            return false;
        }

        waitEvent(timeout);

        auto expired_cbs = getExpiredCallBacks();
        for (const auto& cb : expired_cbs) {
            runTask(cb);
        }
        return true;
    }

    void IOContext::waitEvent(std::optional<std::chrono::system_clock::duration> timeout) {
        struct io_uring_cqe* cqe = nullptr;
        {
            struct __kernel_timespec ts{};
            struct __kernel_timespec* tsp = nullptr;
            if (timeout) {
                tsp = &(ts = durationToKernelTimespec(*timeout));
            } else {
                tsp = nullptr;
            }
            int res = io_uring_submit_and_wait_timeout(&uring_, &cqe, 1, tsp, nullptr);
            if (res == -ETIME) {
                return;
            }
            if (res < 0) [[unlikely]] {
                if (res != -EINTR) {
                    throw std::system_error(-res, std::system_category());
                }
                return;
            }
        }

        unsigned head{};
        unsigned num{};
        io_uring_for_each_cqe(&uring_, head, cqe) {
            auto* data = static_cast<UringOp::UringData*>(io_uring_cqe_get_data(cqe));
            data->res_ = cqe->res;
            schedule(data->fiber_);
            ++num;
        }
        io_uring_cq_advance(&uring_, num);
        pending_ops_ -= static_cast<std::size_t>(num);
    }

    IOContext::Task IOContext::getTask(Func const& func) {
        Task task = nullptr;
        if (!free_tasks_.empty()) {
            task = free_tasks_.front();
            free_tasks_.pop();
            task->reset(func);
        } else {
            task = Fiber::newFiber(func);
        }
        return task;
    }

    void IOContext::runTask(Task task) {
        task->resume();

        auto state = task->state_;
        if (state == Fiber::READY) {
            schedule(task);
        } else if (state == Fiber::TERM || state == Fiber::EXCEPT) {
            free_tasks_.push(task);
        }
    }

    void IOContext::schedule(Task task) { ready_tasks_.push(task); }

    void IOContext::spawn(Func const& func) {
        assertThat(t_context);
        t_context->schedule(func);
    }
    void IOContext::spawn(Task task) {
        assertThat(t_context);
        t_context->schedule(task);
    }

} // namespace sylar
