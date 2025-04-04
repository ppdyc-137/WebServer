#pragma once

#include "file.h"
#include "stream.h"
#include "util.h"

namespace sylar {
    struct FileStream : Stream {
        explicit FileStream(FileHandle file) : file_(std::move(file)) {}

        std::size_t raw_read(std::span<char> buffer) override {
            return static_cast<size_t>(checkRetUring(file_read(file_, buffer)));
        }

        std::size_t raw_write(std::span<char const> buffer) override {
            return static_cast<size_t>(checkRetUring(file_write(file_, buffer)));
        }

        void raw_close() override { file_close(std::move(file_)); }

        FileHandle release() noexcept { return std::move(file_); }
        FileHandle& get() noexcept { return file_; }

    private:
        FileHandle file_;
    };

} // namespace sylar
