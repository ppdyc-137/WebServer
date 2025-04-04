#pragma once

#include "socket.h"
#include "stream.h"
#include "util.h"

namespace sylar {
    struct SocketStream : Stream {
        explicit SocketStream(SocketHandle file) : file_(std::move(file)) {}

        std::size_t raw_read(std::span<char> buffer) override {
            return static_cast<size_t>(checkRetUring(socket_read(file_, buffer)));
        }

        std::size_t raw_write(std::span<char const> buffer) override {
            return static_cast<size_t>(checkRetUring(socket_write(file_, buffer)));
        }

        SocketHandle release() noexcept { return std::move(file_); }
        SocketHandle& get() noexcept { return file_; }

    private:
        SocketHandle file_;
    };

} // namespace sylar
