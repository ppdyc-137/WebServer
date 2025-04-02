#pragma once

#include "fiber.h"
#include "util.h"

#include <cstdint>
#include <liburing.h>
#include <queue>
#include <spdlog/spdlog.h>

namespace sylar {
    class IOManager {
    public:
        using Func = std::function<void()>;
        using Task = Fiber*;

        IOManager();
        ~IOManager();

        void execute();
        void schedule(Func const& func) {
            Task task = nullptr;
            if (!free_tasks_.empty()) {
                task = free_tasks_.front();
                free_tasks_.pop();
                task->reset(func);
            } else {
                task = Fiber::newFiber(func);
            }
            ready_tasks_.push(task);
        }

        static IOManager* getCurrentScheduler() { return t_current_scheduler; }
        static Fiber* getCurrentSchedulerFiber() { return &t_current_scheduler_fiber; }
        uint64_t getPendingOps() { return pending_ops_; }

    private:
        friend struct UringOp;
        struct io_uring_sqe* getSqe();

        void submit() { io_uring_submit(&uring_); }

        static constexpr int RING_SIZE = 256;
        io_uring uring_{};

        std::atomic<uint64_t> pending_ops_;

        std::queue<Task> ready_tasks_;
        std::queue<Task> free_tasks_;

        static inline thread_local IOManager* t_current_scheduler{};
        static inline thread_local Fiber t_current_scheduler_fiber{};
    };

    // NOLINTBEGIN
    struct UringOp {
        UringOp() : fiber_(Fiber::getCurrentFiber()) {
            SYLAR_ASSERT(fiber_);
            sqe_ = IOManager::getCurrentScheduler()->getSqe();
            io_uring_sqe_set_data(sqe_, this);
        }

        UringOp(UringOp&&) = delete;

        static UringOp&& link_ops(UringOp&& lhs, UringOp&&) {
            lhs.sqe_->flags |= IOSQE_IO_LINK;
            return std::move(lhs);
        }

        struct io_uring_sqe* getSqe() const noexcept { return sqe_; }

        int res_;

    private:
        friend class IOManager;
        struct io_uring_sqe* sqe_;
        Fiber* fiber_;

        void yield() {
            Fiber::yield();
            if (res_ < 0) {
                errno = -res_;
                // res_ = -1;
            }
        }

    public:
        UringOp&& prep_openat(int dirfd, char const* path, int flags, mode_t mode) && {
            io_uring_prep_openat(sqe_, dirfd, path, flags, mode);
            yield();
            spdlog::debug("{}, ret: {}", __PRETTY_FUNCTION__, res_);
            return std::move(*this);
        }

        UringOp&& prep_socket(int domain, int type, int protocol, unsigned int flags) && {
            io_uring_prep_socket(sqe_, domain, type, protocol, flags);
            yield();
            spdlog::debug("{}, ret: {}", __PRETTY_FUNCTION__, res_);
            return std::move(*this);
        }

        UringOp&& prep_accept(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) && {
            io_uring_prep_accept(sqe_, fd, addr, addrlen, flags);
            yield();
            spdlog::debug("{}, ret: {}", __PRETTY_FUNCTION__, res_);
            return std::move(*this);
        }

        UringOp&& prep_connect(int fd, const struct sockaddr* addr, socklen_t addrlen) && {
            io_uring_prep_connect(sqe_, fd, addr, addrlen);
            yield();
            spdlog::debug("{}, ret: {}", __PRETTY_FUNCTION__, res_);
            return std::move(*this);
        }

        UringOp&& prep_read(int fd, void* buf, unsigned int len, std::uint64_t offset) && {
            io_uring_prep_read(sqe_, fd, buf, len, offset);
            yield();
            spdlog::debug("{}, ret: {}", __PRETTY_FUNCTION__, res_);
            return std::move(*this);
        }

        UringOp&& prep_write(int fd, const void* buf, unsigned int len, std::uint64_t offset) && {
            io_uring_prep_write(sqe_, fd, buf, len, offset);
            yield();
            spdlog::debug("{}, ret: {}", __PRETTY_FUNCTION__, res_);
            return std::move(*this);
        }

        UringOp&& prep_recv(int fd, void* buf, size_t len, int flags) && {
            io_uring_prep_recv(sqe_, fd, buf, len, flags);
            yield();
            spdlog::debug("{}, ret: {}", __PRETTY_FUNCTION__, res_);
            return std::move(*this);
        }

        UringOp&& prep_send(int fd, const void* buf, size_t len, int flags) && {
            io_uring_prep_send(sqe_, fd, buf, len, flags);
            yield();
            spdlog::debug("{}, ret: {}", __PRETTY_FUNCTION__, res_);
            return std::move(*this);
        }

        UringOp&& prep_close(int fd) && {
            io_uring_prep_close(sqe_, fd);
            yield();
            spdlog::debug("{}, ret: {}", __PRETTY_FUNCTION__, res_);
            return std::move(*this);
        }

        // NOLINTEND
    };

} // namespace sylar
