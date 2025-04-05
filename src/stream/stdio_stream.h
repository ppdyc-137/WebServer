#pragma once

#include "file/file.h"
#include "stream.h"
#include "util.h"
#include <cstddef>

namespace sylar {
    struct StdioStream : Stream {
        explicit StdioStream(FileHandle& fileIn, FileHandle& fileOut) : file_int_(fileIn), file_out_(fileOut) {}

        std::size_t raw_read(std::span<char> buffer) override {
            return static_cast<std::size_t>(checkRetUring(file_read(file_int_, buffer)));
        }

        std::size_t raw_write(std::span<char const> buffer) override {
            return static_cast<std::size_t>(checkRetUring(file_write(file_out_, buffer)));
        }

        FileHandle& in() const noexcept { return file_int_; }
        FileHandle& out() const noexcept { return file_out_; }

    private:
        FileHandle& file_int_;
        FileHandle& file_out_;
    };

    inline OwningStream& stdio() {
        static thread_local OwningStream stream = make_stream<StdioStream>(stdin_handle(), stdout_handle());
        return stream;
    }

} // namespace sylar
