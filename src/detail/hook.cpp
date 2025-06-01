#include "hook.h"
#include "processor.h"
#include "uring_op.h"

#include <cerrno>
#include <fcntl.h>
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

#define HOOK_FUNCTION_IMPL(name, ...)                                                                                  \
    auto res = sylar::UringOp().prep_##name(__VA_ARGS__).await();                                                      \
    if (res < 0) {                                                                                                     \
        errno = -res;                                                                                                  \
        res = -1;                                                                                                      \
        spdlog::debug(#name ": {}", strerror(errno));                                                                  \
    }                                                                                                                  \
    return res

#define HOOK_SYSCALL(name, ...)                                                                                        \
    if (!hook_enable || !sylar::Processor::getProcessor()) {                                                           \
        return name##_f(__VA_ARGS__);                                                                                  \
    }                                                                                                                  \
    HOOK_FUNCTION_IMPL(name, __VA_ARGS__)

int socket(int domain, int type, int protocol) { HOOK_SYSCALL(socket, domain, type, protocol); }

int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    HOOK_SYSCALL(connect, sockfd, addr, addrlen);
}

int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) { HOOK_SYSCALL(accept, sockfd, addr, addrlen); }

ssize_t read(int fd, void* buf, size_t count) { HOOK_SYSCALL(read, fd, buf, static_cast<unsigned int>(count)); }

ssize_t recv(int fd, void* buf, size_t n, int flags) { HOOK_SYSCALL(recv, fd, buf, n, flags); }

ssize_t send(int fd, const void* buf, size_t n, int flags) { HOOK_SYSCALL(send, fd, buf, n, flags); }

ssize_t write(int fd, const void* buf, size_t count) { HOOK_SYSCALL(write, fd, buf, static_cast<unsigned int>(count)); }

int close(int fd) { HOOK_SYSCALL(close, fd); }
