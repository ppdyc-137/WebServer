#pragma once

#include "uring_op.h"
#include "util.h"

#include <cstdint>
#include <dirent.h>
#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>
#include <unistd.h>

#include <span>
#include <utility>

namespace sylar {
    struct [[nodiscard]] FileHandle {
        FileHandle() noexcept = default;

        explicit FileHandle(int fileNo) noexcept : fd_(fileNo) {}

        int fileNo() const noexcept { return fd_; }

        int releaseFile() noexcept { return std::exchange(fd_, -1); }

        explicit operator bool() const noexcept { return fd_ > 0; }

        FileHandle(FileHandle&& that) noexcept : fd_(that.releaseFile()) {}

        FileHandle& operator=(FileHandle&& that) noexcept {
            std::swap(fd_, that.fd_);
            return *this;
        }

        ~FileHandle() {
            if (fd_ > 0) {
                close(fd_);
            }
        }

    protected:
        int fd_{-1};
    };

    template <int fd>
    FileHandle& stdFileHandle() {
        static FileHandle handle(fd);
        return handle;
    }
    inline FileHandle& stdin_handle() { return stdFileHandle<STDIN_FILENO>(); }
    inline FileHandle& stdout_handle() { return stdFileHandle<STDOUT_FILENO>(); }

    enum class OpenMode : int {
        Read = O_RDONLY,
        Write = O_WRONLY | O_TRUNC | O_CREAT,
        ReadWrite = O_RDWR | O_CREAT,
        Append = O_WRONLY | O_APPEND | O_CREAT,
        Directory = O_RDONLY | O_DIRECTORY,
    };

    inline FileHandle file_open(const std::filesystem::path& path, OpenMode mode, mode_t access = 0644) {
        int flags = static_cast<int>(mode);
        int fd = UringOp().prep_openat(AT_FDCWD, path.c_str(), flags, access).await();
        checkRetUring(fd);
        return FileHandle(fd);
    }

    inline void file_close(FileHandle file) { checkRetUring(UringOp().prep_close(file.releaseFile()).await()); }

    inline int file_read(FileHandle& file, std::span<char> buffer, uint64_t offset = static_cast<uint64_t>(-1)) {
        return checkRetUring(
            UringOp()
                .prep_read(file.fileNo(), buffer.data(), static_cast<unsigned int>(buffer.size()), offset)
                .await());
    }

    inline int file_write(FileHandle& file, std::span<char const> buffer, uint64_t offset = static_cast<uint64_t>(-1)) {
        return checkRetUring(
            UringOp()
                .prep_write(file.fileNo(), buffer.data(), static_cast<unsigned int>(buffer.size()), offset)
                .await());
    }

} // namespace sylar
