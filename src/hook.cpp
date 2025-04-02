#include "hook.h"
#include "iomanager.h"

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

int socket(int domain, int type, int protocol) {
    // if (!hook_enable) {
    //     return socket_f(domain, type, protocol);
    // }
    return sylar::UringOp().prep_socket(domain, type, protocol, 0).res_;
}

int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    // if (!hook_enable) {
    //     return connect_f(sockfd, addr, addrlen);
    // }
    return sylar::UringOp().prep_connect(sockfd, addr, addrlen).res_;
}

int accept(int sockfd, struct sockaddr* __restrict addr, socklen_t* __restrict addr_len) {
    // if (!hook_enable) {
    //     return accept_f(sockfd, addr, addr_len);
    // }
    return sylar::UringOp().prep_accept(sockfd, addr, addr_len, 0).res_;
}

ssize_t read(int fd, void* buf, size_t nbytes) {
    // if (!hook_enable) {
    //     return read_f(fd, buf, nbytes);
    // }
    return sylar::UringOp().prep_read(fd, buf, static_cast<unsigned int>(nbytes), 0).res_;
}

ssize_t recv(int fd, void* buf, size_t n, int flags) {
    // if (!hook_enable) {
    //     return recv_f(fd, buf, n, flags);
    // }
    return sylar::UringOp().prep_recv(fd, buf, n, flags).res_;
}

ssize_t send(int fd, const void* buf, size_t n, int flags) {
    // if (!hook_enable) {
    //     return send_f(fd, buf, n, flags);
    // }
    return sylar::UringOp().prep_send(fd, buf, n, flags).res_;
}

ssize_t write(int fd, const void* buf, size_t count) {
    // if (!hook_enable) {
    //     return write_f(fd, buf, count);
    // }
    return sylar::UringOp().prep_write(fd, buf, static_cast<unsigned int>(count), 0).res_;
}

int close(int fd) {
    // if (!hook_enable) {
    //     return close_f(fd);
    // }
    return sylar::UringOp().prep_close(fd).res_;
}
