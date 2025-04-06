#pragma once

#include "detail/fiber.h"
#include "io_context.h"
#include "util.h"

#include <liburing.h>
#include <optional>
#include <spdlog/spdlog.h>

namespace sylar {
    // NOLINTBEGIN
    struct [[nodiscard]] UringOp {
        using timeout_type = std::optional<std::chrono::system_clock::duration>;
        UringOp(timeout_type timeout = std::nullopt) : timeout_(timeout) {
            assertThat(Fiber::getCurrentFiber());
            sqe_ = IOContext::getCurrentContext()->getSqe();
            io_uring_sqe_set_data(sqe_, &op_data_);
        }
        ~UringOp() { assertThat(yield_); }

        UringOp(UringOp&&) = delete;

    private:
        friend class IOContext;

        struct UringData {
            int res_{};
            Fiber* fiber_{Fiber::getCurrentFiber()};
        };

        void prep_link_timeout(UringData* data, struct __kernel_timespec* tp) {
            sqe_->flags |= IOSQE_IO_LINK;

            auto sqe = IOContext::getCurrentContext()->getSqe();
            io_uring_sqe_set_data(sqe, data);
            io_uring_prep_link_timeout(sqe, tp, IORING_TIMEOUT_BOOTTIME);

            spdlog::debug("{}", __PRETTY_FUNCTION__);
        }
        void prep_cancel(UringData* data) {
            auto sqe = IOContext::getCurrentContext()->getSqe();
            io_uring_prep_cancel(sqe, this, IORING_ASYNC_CANCEL_ALL);
            io_uring_sqe_set_data(sqe, data);

            spdlog::debug("{}", __PRETTY_FUNCTION__);
        }

        bool yield_{false};
        struct io_uring_sqe* sqe_;
        timeout_type timeout_;

        UringData op_data_;

    public:
        int await() && {
            if (timeout_) {
                bool canceled_{false};
                auto ts = durationToKernelTimespec(*timeout_);
                UringData timeout_data_;
                UringData cancel_data_;

                prep_link_timeout(&timeout_data_, &ts);
                for (int i = 0; i < 2; i++) {
                    Fiber::yield();
                    if (timeout_data_.res_ == -ETIME && !canceled_) {
                        canceled_ = true;
                        prep_cancel(&cancel_data_);
                        Fiber::yield();
                    }
                }
            } else {
                Fiber::yield();
            }
            yield_ = true;
            return op_data_.res_;
        }

        [[nodiscard("need to call await")]]
        UringOp&& prep_openat(int dirfd, char const* path, int flags, mode_t mode) && {
            io_uring_prep_openat(sqe_, dirfd, path, flags, mode);
            spdlog::debug("{}", __PRETTY_FUNCTION__);
            return std::move(*this);
        }

        [[nodiscard("need to call await")]]
        UringOp&& prep_socket(int domain, int type, int protocol, unsigned int flags = 0) && {
            io_uring_prep_socket(sqe_, domain, type, protocol, flags);
            spdlog::debug("{}", __PRETTY_FUNCTION__);
            return std::move(*this);
        }

        [[nodiscard("need to call await")]]
        UringOp&& prep_accept(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags = 0) && {
            io_uring_prep_accept(sqe_, fd, addr, addrlen, flags);
            spdlog::debug("{}", __PRETTY_FUNCTION__);
            return std::move(*this);
        }

        [[nodiscard("need to call await")]]
        UringOp&& prep_connect(int fd, const struct sockaddr* addr, socklen_t addrlen) && {
            io_uring_prep_connect(sqe_, fd, addr, addrlen);
            spdlog::debug("{}", __PRETTY_FUNCTION__);
            return std::move(*this);
        }

        [[nodiscard("need to call await")]]
        UringOp&& prep_read(int fd, void* buf, unsigned int len, std::uint64_t offset = static_cast<uint64_t>(-1)) && {
            io_uring_prep_read(sqe_, fd, buf, len, offset);
            spdlog::debug("{}", __PRETTY_FUNCTION__);
            return std::move(*this);
        }

        [[nodiscard("need to call await")]]
        UringOp&& prep_write(int fd, const void* buf, unsigned int len,
                             std::uint64_t offset = static_cast<uint64_t>(-1)) && {
            io_uring_prep_write(sqe_, fd, buf, len, offset);
            spdlog::debug("{}", __PRETTY_FUNCTION__);
            return std::move(*this);
        }

        [[nodiscard("need to call await")]]
        UringOp&& prep_recv(int fd, void* buf, size_t len, int flags) && {
            io_uring_prep_recv(sqe_, fd, buf, len, flags);
            spdlog::debug("{}", __PRETTY_FUNCTION__);
            return std::move(*this);
        }

        [[nodiscard("need to call await")]]
        UringOp&& prep_send(int fd, const void* buf, size_t len, int flags) && {
            io_uring_prep_send(sqe_, fd, buf, len, flags);
            spdlog::debug("{}", __PRETTY_FUNCTION__);
            return std::move(*this);
        }

        [[nodiscard("need to call await")]]
        UringOp&& prep_close(int fd) && {
            io_uring_prep_close(sqe_, fd);
            spdlog::debug("{}", __PRETTY_FUNCTION__);
            return std::move(*this);
        }

        [[nodiscard("need to call await")]]
        UringOp&& prep_link_timeout(struct __kernel_timespec* ts, unsigned int flags) && {
            io_uring_prep_link_timeout(sqe_, ts, flags);
            spdlog::debug("{}", __PRETTY_FUNCTION__);
            return std::move(*this);
        }

        // NOLINTEND
    };
} // namespace sylar
