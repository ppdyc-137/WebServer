#include "hook.h"
#include "file_descriptor.h"
#include "iomanager.h"
#include "scheduler.h"

#include <cerrno>
#include <fcntl.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

namespace {
    thread_local bool hook_enable = false;
}

namespace sylar {
    bool isHookEnable() { return hook_enable; }
    void setHookEnable(bool enable) { hook_enable = enable; }
} // namespace sylar

constexpr uint64_t K = 1000;
unsigned int sleep(unsigned int seconds) {
    if (!hook_enable) {
        return sleep_f(seconds);
    }
    auto fiber = sylar::Fiber::getCurrentFiber();
    sylar::IOManager::getCurrentScheduler()->addTimer(
        seconds * K, [fiber = std::move(fiber)]() { sylar::IOManager::getCurrentScheduler()->schedule(fiber); }, false);
    sylar::Fiber::yield(sylar::Fiber::HOLD);
    return 0;
}

int socket(int domain, int type, int protocol) {
    if (!hook_enable) {
        return socket_f(domain, type, protocol);
    }
    int fd = socket_f(domain, type, protocol);
    sylar::FDManager::getInstance().get(fd, true);
    return fd;
}

struct TimerInfo {
    bool timed_out_{false};
};

int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout) {
    if (!hook_enable) {
        return connect_f(sockfd, addr, addrlen);
    }
    auto file_discriptor = sylar::FDManager::getInstance().get(sockfd);
    if (!file_discriptor || !file_discriptor->is_socket_) {
        return connect_f(sockfd, addr, addrlen);
    }

    {
        auto ret = connect_f(sockfd, addr, addrlen);
        if (ret == 0 || errno != EINPROGRESS) {
            return ret;
        }
    }

    {
        auto* scheduler = sylar::IOManager::getCurrentScheduler();
        auto timer_info = std::make_shared<TimerInfo>();
        auto timer_info_weak = std::weak_ptr(timer_info);
        if (timeout != sylar::TIMEOUT_INFINITY) {
            scheduler->addConditionTimer(
                timeout,
                [scheduler, sockfd, timer_info_weak]() {
                    timer_info_weak.lock()->timed_out_ = true;
                    scheduler->cancelEvent(sockfd, EPOLLOUT);
                },
                timer_info_weak, false);
        }

        auto ret = scheduler->addEvent(sockfd, EPOLLOUT);
        if (ret) {
            sylar::Fiber::yield(sylar::Fiber::HOLD);
            if (timer_info->timed_out_) {
                errno = ETIMEDOUT;
                return -1;
            }
        } else {
            spdlog::error("connect: addEvent({}, EPOLLOUT) error", sockfd);
        }
    }

    int result = 0;
    socklen_t result_len = sizeof(result);
    auto ret = getsockopt_f(sockfd, SOL_SOCKET, SO_ERROR, &result, &result_len);
    if (ret < 0) {
        return -1;
    }
    if (result == 0) {
        return 0;
    }
    errno = result;
    return -1;
}

int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    return connect(sockfd, addr, addrlen, sylar::TIMEOUT_INFINITY);
}

namespace {
    template <typename Func, typename... Args>
    ssize_t doIO(int fd, Func original_func, const char* name, EPOLL_EVENTS event, Args... args) {
        if (!hook_enable) {
            return original_func(fd, args...);
        }

        auto file_discriptor = sylar::FDManager::getInstance().get(fd);
        if (!file_discriptor || !file_discriptor->is_socket_ || !file_discriptor->non_blocking_) {
            return original_func(fd, args...);
        }

        uint64_t timeout{};
        if (event == EPOLLIN) {
            timeout = file_discriptor->recv_timeout_;
        } else {
            timeout = file_discriptor->send_timeout_;
        }

        while (true) {
            auto ret = original_func(fd, args...);
            if (ret >= 0) {
                return ret;
            }
            if (errno == EINTR) {
                continue;
            }
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                return -1;
            }

            auto* scheduler = sylar::IOManager::getCurrentScheduler();
            auto timer_info = std::make_shared<TimerInfo>();
            auto timer_info_weak = std::weak_ptr(timer_info);
            if (timeout != sylar::TIMEOUT_INFINITY) {
                scheduler->addConditionTimer(
                    timeout,
                    [scheduler, fd, timer_info_weak, event]() {
                        timer_info_weak.lock()->timed_out_ = true;
                        scheduler->cancelEvent(fd, event);
                    },
                    timer_info_weak, false);
            }

            bool success = scheduler->addEvent(fd, event);
            if (success) {
                sylar::Fiber::yield(sylar::Fiber::HOLD);
                if (timer_info->timed_out_) {
                    errno = ETIMEDOUT;
                    return -1;
                }
            } else {
                spdlog::error("{}: addEvent({}, {}) error", name, fd, sylar::strevent(event));
            }
        }
    }
} // namespace

int accept(int sockfd, struct sockaddr* __restrict addr, socklen_t* __restrict addr_len) {
    auto fd = static_cast<int>(doIO(sockfd, accept_f, "accept", EPOLLIN, addr, addr_len));
    sylar::FDManager::getInstance().get(fd, true);
    return fd;
}

ssize_t read(int fd, void* buf, size_t nbytes) { return doIO(fd, read_f, "read", EPOLLIN, buf, nbytes); }

ssize_t readv(int fd, const struct iovec* iovec, int count) {
    return doIO(fd, readv_f, "readv", EPOLLIN, iovec, count);
}

ssize_t recv(int fd, void* buf, size_t n, int flags) { return doIO(fd, recv_f, "recv", EPOLLIN, buf, n, flags); }

ssize_t recvfrom(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t* addrlen) {
    return doIO(sockfd, recvfrom_f, "recvfrom", EPOLLIN, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr* msg, int flags) {
    return doIO(sockfd, recvmsg_f, "recvmsg", EPOLLIN, msg, flags);
}

ssize_t write(int fd, const void* buf, size_t count) { return doIO(fd, write_f, "write", EPOLLOUT, buf, count); }

ssize_t writev(int fd, const struct iovec* iov, int iovcnt) {
    return doIO(fd, writev_f, "writev", EPOLLOUT, iov, iovcnt);
}

ssize_t send(int s, const void* msg, size_t len, int flags) {
    return doIO(s, send_f, "send", EPOLLOUT, msg, len, flags);
}

ssize_t sendto(int s, const void* msg, size_t len, int flags, const struct sockaddr* to, socklen_t tolen) {
    return doIO(s, sendto_f, "sendto", EPOLLOUT, msg, len, flags, to, tolen);
}

ssize_t sendmsg(int s, const struct msghdr* msg, int flags) {
    return doIO(s, sendmsg_f, "sendmsg", EPOLLOUT, msg, flags);
}

int close(int fd) {
    if (!hook_enable) {
        return close_f(fd);
    }

    auto file_discriptor = sylar::FDManager::getInstance().get(fd);
    if (file_discriptor) {
        auto* manager = sylar::IOManager::getCurrentScheduler();
        if (manager) {
            manager->cancelAll(fd);
        }
        sylar::FDManager::getInstance().del(fd);
    }
    return close_f(fd);
}

int getsockopt(int sockfd, int level, int optname, void* optval, socklen_t* optlen) {
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen) {
    if (!hook_enable) {
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
    if (level == SOL_SOCKET && (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO)) {
        auto file_discriptor = sylar::FDManager::getInstance().get(sockfd);
        if (file_discriptor) {
            const auto* tv = static_cast<const timeval*>(optval);
            uint64_t timeout = (static_cast<uint64_t>(tv->tv_sec) * K) + (static_cast<uint64_t>(tv->tv_usec) / K);
            if (optname == SO_RCVTIMEO) {
                file_discriptor->recv_timeout_ = timeout;
            } else {
                file_discriptor->send_timeout_ = timeout;
            }
        }
    }
    return setsockopt_f(sockfd, level, optname, optval, optlen);
}

// TODO
//  int fcntl(int fd, int cmd, ...);
//  int ioctl(int fd, unsigned long request, ...);
