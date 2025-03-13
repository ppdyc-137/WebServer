#include "file_descriptor.h"
#include "hook.h"
#include "mutex.h"
#include "util.h"

#include <cstring>
#include <fcntl.h>
#include <memory>
#include <sys/stat.h>

namespace sylar {
    FileDiscriptor::FileDiscriptor(int fd) : fd_(fd) {
        struct stat fd_stat{};
        SYLAR_ASSERT2(fstat(fd, &fd_stat) == 0, strerror(errno));
        is_socket_ = S_ISSOCK(fd_stat.st_mode);

        int flags = fcntl_f(fd, F_GETFL, 0);
        SYLAR_ASSERT2(flags >= 0, strerror(errno));
        non_blocking_ = flags & O_NONBLOCK;
        if (is_socket_ && !non_blocking_) {
            fcntl_f(fd, F_SETFL, flags | O_NONBLOCK);
            non_blocking_ = true;
        }
    }

    std::shared_ptr<FileDiscriptor> FDManager::get(int fd, bool create) {
        if (fd < 0) {
            return nullptr;
        }
        {
            ReadLockGuard lock(mutex_);
            if (fds_.contains(fd)) {
                return fds_[fd];
            }
        }
        if (!create) {
            return nullptr;
        }
        {
            WriteLockGuard lock(mutex_);
            auto file_discriptor = std::make_shared<FileDiscriptor>(fd);
            fds_.emplace(fd, file_discriptor);
            return file_discriptor;
        }
    }

    void FDManager::del(int fd) {
        WriteLockGuard lock(mutex_);
        fds_.erase(fd);
    }

} // namespace sylar
