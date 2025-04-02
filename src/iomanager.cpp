#include "iomanager.h"
#include "fiber.h"
#include "hook.h"
#include "util.h"
#include <cassert>
#include <cstring>
#include <fcntl.h>
#include <liburing.h>
#include <spdlog/spdlog.h>
#include <unistd.h>

namespace sylar {
    IOManager::IOManager() {
        auto ret = io_uring_queue_init(RING_SIZE, &uring_, 0);
        t_current_scheduler = this;
        Fiber::t_current_fiber = &t_current_scheduler_fiber;
        setHookEnable(true);
        SYLAR_ASSERT2(ret == 0, strerror(-ret));
    }

    IOManager::~IOManager() { io_uring_queue_exit(&uring_); }

    struct io_uring_sqe* IOManager::getSqe() {
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

    void IOManager::execute() {
        while (true) {
            while (!ready_tasks_.empty()) {
                auto* task = ready_tasks_.front();
                ready_tasks_.pop();

                task->resume();

                auto state = task->getState();
                if (state == Fiber::TERM || state == Fiber::EXCEPT) {
                    free_tasks_.push(task);
                }
            }

            struct io_uring_cqe* cqe = nullptr;
            // io_uring_submit(&uring_);
            // while (true) {
            //     int ret = io_uring_peek_cqe(&uring_, &cqe);
            //     if (ret == 0) {
            //         break;
            //     }
            // }
            {
                int res = io_uring_submit_and_wait_timeout(&uring_, &cqe, 1, nullptr, nullptr);
                if (res == -ETIME) {
                    continue;
                }
                if (res < 0) [[unlikely]] {
                    if (res != -EINTR) {
                        spdlog::error("io_uring_wait: {}", strerror(-res));
                    }
                    continue;
                }
            }

            unsigned head{};
            unsigned num{};
            io_uring_for_each_cqe(&uring_, head, cqe) {
                auto* op = static_cast<UringOp*>(io_uring_cqe_get_data(cqe));
                op->res_ = cqe->res;
                auto* task = op->fiber_;
                ready_tasks_.push(task);
                ++num;
            }
            io_uring_cq_advance(&uring_, num);
            pending_ops_ -= static_cast<std::size_t>(num);
        }
    }

} // namespace sylar
