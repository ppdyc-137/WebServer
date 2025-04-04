#pragma once

#include "fiber.h"
#include "io_context.h"
#include "util.h"

#include <liburing.h>
#include <spdlog/spdlog.h>
namespace sylar {
    // NOLINTBEGIN
    struct [[nodiscard]] UringOp {
        UringOp() : fiber_(Fiber::getCurrentFiber()) {
            myAssert(fiber_);
            sqe_ = IOContext::getCurrentContext()->getSqe();
            io_uring_sqe_set_data(sqe_, this);
        }
        ~UringOp() { myAssert(yield_); }

        UringOp(UringOp&&) = delete;

        [[nodiscard("need to call await")]]
        static UringOp&& link_ops(UringOp&& lhs, UringOp&& rhs) {
            lhs.sqe_->flags |= IOSQE_IO_LINK;
            io_uring_sqe_set_data(rhs.sqe_, nullptr);
            rhs.yield_ = true;
            return std::move(lhs);
        }

    private:
        friend class IOContext;

        int res_;
        bool yield_{false};
        struct io_uring_sqe* sqe_;
        Fiber* fiber_;

    public:
        int await() && {
            Fiber::yield();
            yield_ = true;
            return res_;
        }

        [[nodiscard("need to call await")]]
        UringOp&& prep_socket(int domain, int type, int protocol, unsigned int flags = 0) && {
            io_uring_prep_socket(sqe_, domain, type, protocol, flags);
            return std::move(*this);
        }

        [[nodiscard("need to call await")]]
        UringOp&& prep_accept(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags = 0) && {
            io_uring_prep_accept(sqe_, fd, addr, addrlen, flags);
            return std::move(*this);
        }

        [[nodiscard("need to call await")]]
        UringOp&& prep_connect(int fd, const struct sockaddr* addr, socklen_t addrlen) && {
            io_uring_prep_connect(sqe_, fd, addr, addrlen);
            return std::move(*this);
        }

        [[nodiscard("need to call await")]]
        UringOp&& prep_read(int fd, void* buf, unsigned int len, std::uint64_t offset = 0) && {
            io_uring_prep_read(sqe_, fd, buf, len, offset);
            spdlog::debug("{}", __PRETTY_FUNCTION__);
            return std::move(*this);
        }

        [[nodiscard("need to call await")]]
        UringOp&& prep_write(int fd, const void* buf, unsigned int len, std::uint64_t offset = 0) && {
            io_uring_prep_write(sqe_, fd, buf, len, offset);
            spdlog::debug("{}", __PRETTY_FUNCTION__);
            return std::move(*this);
        }

        [[nodiscard("need to call await")]]
        UringOp&& prep_recv(int fd, void* buf, size_t len, int flags) && {
            io_uring_prep_recv(sqe_, fd, buf, len, flags);
            return std::move(*this);
        }

        [[nodiscard("need to call await")]]
        UringOp&& prep_send(int fd, const void* buf, size_t len, int flags) && {
            io_uring_prep_send(sqe_, fd, buf, len, flags);
            return std::move(*this);
        }

        [[nodiscard("need to call await")]]
        UringOp&& prep_close(int fd) && {
            io_uring_prep_close(sqe_, fd);
            return std::move(*this);
        }
        // NOLINTEND
    };
} // namespace sylar
