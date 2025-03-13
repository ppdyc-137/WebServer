#pragma once

#include "mutex.h"
#include "timer.h"
#include <memory>
#include <unordered_map>

namespace sylar {
    struct FileDiscriptor : public std::enable_shared_from_this<FileDiscriptor> {
        explicit FileDiscriptor(int fd);
        virtual ~FileDiscriptor() = default;

        int fd_;
        bool non_blocking_{false};
        bool is_socket_{false};
        uint64_t recv_timeout_{TIMEOUT_INFINITY};
        uint64_t send_timeout_{TIMEOUT_INFINITY};
    };

    class FDManager {
    public:
        static FDManager& getInstance() {
            static FDManager manager;
            return manager;
        }

        std::shared_ptr<FileDiscriptor> get(int fd, bool create = false);
        void del(int fd);

    private:
        RWMutex mutex_;
        std::unordered_map<int, std::shared_ptr<FileDiscriptor>> fds_;
    };

} // namespace sylar
