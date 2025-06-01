#include "processor.h"
#include "detail/hook.h"
#include "io_context.h"
#include "uring_op.h"
#include "util.h"

#include <chrono>
#include <liburing.h>
#include <spdlog/spdlog.h>
#include <thread>

constexpr std::chrono::system_clock::duration MAX_EVENT_WAIT = std::chrono::milliseconds(10);

namespace sylar {
    Processor::Processor(uint64_t id, bool hook, unsigned int entries) : id_(id) {
        assertThat(t_processor == nullptr);
        t_processor = this;
        checkRetUring(io_uring_queue_init(entries, &uring_, 0));
        Fiber::t_current_fiber = &t_processor_fiber;

        if (hook) {
            setHookEnable(true);
        }
    }

    Processor::~Processor() { io_uring_queue_exit(&uring_); }

    struct io_uring_sqe* Processor::getSqe() {
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

    void Processor::execute() {
        spdlog::debug("Processor {}: Executing", id_);
        while (true) {
            bool has_job = execOnce();
            if (rq_.size() != 0) {
                continue;
            }

            auto size = IOContext::getInstance()->stealTasks(id_, rq_);
            if (size > 0) {
                continue;
            }

            if (!has_job && size == 0) {
                std::this_thread::sleep_for(MAX_EVENT_WAIT);
            }
        }
    }

    bool Processor::execOnce() {
        for (Task task = rq_.pop(); task != nullptr; task = rq_.pop()) {
            execTask(task);
        }

        auto timeout = getNextTriggerDuration();
        if (pending_ops_ == 0 && !timeout) {
            return false;
        }

        std::chrono::system_clock::duration time = MAX_EVENT_WAIT;
        if (timeout) {
            time = min(time, *timeout);
        }

        waitEvent(time);

        auto expired_cbs = getExpiredCallBacks();
        for (const auto& cb : expired_cbs) {
            execTask(cb);
        }
        return true;
    }

    void Processor::waitEvent(std::chrono::system_clock::duration timeout) {
        struct io_uring_cqe* cqe = nullptr;
        {
            struct __kernel_timespec ts = durationToKernelTimespec(timeout);
            int res = io_uring_submit_and_wait_timeout(&uring_, &cqe, 1, &ts, nullptr);
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
            emplaceTask(data->fiber_);
            ++num;
        }
        io_uring_cq_advance(&uring_, num);
        pending_ops_ -= static_cast<std::size_t>(num);
    }

    void Processor::execTask(Task task) {
        task->resume();

        auto state = task->state_;
        if (state == Fiber::READY) {
            emplaceTask(task);
        } else if (state == Fiber::TERM || state == Fiber::EXCEPT) {
            rq_.emplace_free(task);
        }
    }

} // namespace sylar
