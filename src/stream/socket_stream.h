#pragma once

#include "socket.h"
#include "stream.h"
#include "util.h"

namespace sylar {
    struct SocketStream : Stream {
        explicit SocketStream(SocketHandle file) : file_(std::move(file)) {}

        std::size_t raw_read(std::span<char> buffer) override {
            return static_cast<size_t>(checkRetUring(socket_read(file_, buffer, timeout_)));
        }

        std::size_t raw_write(std::span<char const> buffer) override {
            return static_cast<size_t>(checkRetUring(socket_write(file_, buffer, timeout_)));
        }
        void raw_timeout(UringOp::timeout_type timeout) override { timeout_ = timeout; }

        SocketHandle release() noexcept { return std::move(file_); }
        SocketHandle& get() noexcept { return file_; }

    private:
        UringOp::timeout_type timeout_;
        SocketHandle file_;
    };

} // namespace sylar
