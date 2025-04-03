#include "hook.h"
#include "io_context.h"
#include "uring_op.h"

#include <cerrno>
#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>

#define HOOK_FUNCTION_IMPL(name, ...) \
    auto res = sylar::UringOp().prep_##name(__VA_ARGS__).await(); \
    if (res < 0) { \
        errno = -res; \
        res = -1; \
        spdlog::debug(#name ": {}", strerror(errno)); \
    } \
    return res

#define HOOK_SYSCALL(name, ...) \
    if (!sylar::IOContext::getCurrentContext()) { \
        return name##_f(__VA_ARGS__); \
    } \
    HOOK_FUNCTION_IMPL(name, __VA_ARGS__)

int socket(int domain, int type, int protocol) {
    HOOK_SYSCALL(socket, domain, type, protocol);
}

int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    HOOK_SYSCALL(connect, sockfd, addr, addrlen);
}

int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    HOOK_SYSCALL(accept, sockfd, addr, addrlen);
}

ssize_t read(int fd, void* buf, size_t count) {
    HOOK_SYSCALL(read, fd, buf, static_cast<unsigned int>(count));
}

ssize_t recv(int fd, void* buf, size_t n, int flags) {
    HOOK_SYSCALL(recv, fd, buf, n, flags);
}

ssize_t send(int fd, const void* buf, size_t n, int flags) {
    HOOK_SYSCALL(send, fd, buf, n, flags);
}

ssize_t write(int fd, const void* buf, size_t count) {
    HOOK_SYSCALL(write, fd, buf, static_cast<unsigned int>(count));
}

int close(int fd) {
    HOOK_SYSCALL(close, fd);
}

// int socket(int domain, int type, int protocol) {
//     if (!sylar::IOContext::getCurrentContext()) {
//         return socket_f(domain, type, protocol);
//     }
//     return sylar::UringOp().prep_socket(domain, type, protocol, 0).await();
// }
//
// int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
//     if (!sylar::IOContext::getCurrentContext()) {
//         return connect_f(sockfd, addr, addrlen);
//     }
//     return sylar::UringOp().prep_connect(sockfd, addr, addrlen).await();
// }
//
// int accept(int sockfd, struct sockaddr* __restrict addr, socklen_t* __restrict addr_len) {
//     if (!sylar::IOContext::getCurrentContext()) {
//         return accept_f(sockfd, addr, addr_len);
//     }
//     return sylar::UringOp().prep_accept(sockfd, addr, addr_len, 0).await();
// }
//
// ssize_t read(int fd, void* buf, size_t nbytes) {
//     if (!sylar::IOContext::getCurrentContext()) {
//         return read_f(fd, buf, nbytes);
//     }
//     return sylar::UringOp().prep_read(fd, buf, static_cast<unsigned int>(nbytes), 0).await();
// }
//
// ssize_t recv(int fd, void* buf, size_t n, int flags) {
//     if (!sylar::IOContext::getCurrentContext()) {
//         return recv_f(fd, buf, n, flags);
//     }
//     return sylar::UringOp().prep_recv(fd, buf, n, flags).await();
// }
//
// ssize_t send(int fd, const void* buf, size_t n, int flags) {
//     if (!sylar::IOContext::getCurrentContext()) {
//         return send_f(fd, buf, n, flags);
//     }
//     return sylar::UringOp().prep_send(fd, buf, n, flags).await();
// }
//
// ssize_t write(int fd, const void* buf, size_t count) {
//     if (!sylar::IOContext::getCurrentContext()) {
//         return write_f(fd, buf, count);
//     }
//     return sylar::UringOp().prep_write(fd, buf, static_cast<unsigned int>(count), 0).await();
// }
//
// int close(int fd) {
//     if (!sylar::IOContext::getCurrentContext()) {
//         return close_f(fd);
//     }
//     return sylar::UringOp().prep_close(fd).await();
// }
