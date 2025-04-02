#pragma once

#include <dlfcn.h>
#include <functional>
#include <sys/socket.h>
#include <unistd.h>

namespace sylar {
    bool isHookEnable();
    void setHookEnable(bool enable);
} // namespace sylar

template <typename Func>
struct OriginalFunction {
    explicit OriginalFunction(const char* name) : func_(reinterpret_cast<Func*>(dlsym(RTLD_NEXT, name))) {}

    auto operator()(auto... args) { return func_(args...); }

    std::function<Func> func_{};
};

using sleep_func = unsigned int(unsigned int);
inline auto sleep_f = OriginalFunction<sleep_func>("sleep");

using socket_func = int(int, int, int);
inline auto socket_f = OriginalFunction<socket_func>("socket");

using connect_func = int(int, const struct sockaddr*, socklen_t);
inline auto connect_f = OriginalFunction<connect_func>("connect");

using accept_func = int(int, struct sockaddr*, socklen_t*);
inline auto accept_f = OriginalFunction<accept_func>("accept");

using read_func = ssize_t(int, void*, size_t);
inline auto read_f = OriginalFunction<read_func>("read");

using readv_func = ssize_t(int, const struct iovec*, int);
inline auto readv_f = OriginalFunction<readv_func>("readv");

using recv_func = ssize_t(int, void*, size_t, int);
inline auto recv_f = OriginalFunction<recv_func>("recv");

using recvfrom_func = ssize_t(int, void*, size_t, int, struct sockaddr*, socklen_t*);
inline auto recvfrom_f = OriginalFunction<recvfrom_func>("recvfrom");

using recvmsg_func = ssize_t(int, struct msghdr*, int);
inline auto recvmsg_f = OriginalFunction<recvmsg_func>("recvmsg");

using write_func = ssize_t(int, const void*, size_t);
inline auto write_f = OriginalFunction<write_func>("write");

using writev_func = ssize_t(int, const struct iovec*, int);
inline auto writev_f = OriginalFunction<writev_func>("writev");

using send_func = ssize_t(int, const void*, size_t, int);
inline auto send_f = OriginalFunction<send_func>("send");

using sendto_func = ssize_t(int, const void*, size_t, int, const struct sockaddr*, socklen_t);
inline auto sendto_f = OriginalFunction<sendto_func>("sendto");

using sendmsg_func = ssize_t(int, const struct msghdr*, int);
inline auto sendmsg_f = OriginalFunction<sendmsg_func>("sendmsg");

using close_func = int(int);
inline auto close_f = OriginalFunction<close_func>("close");

using fcntl_func = int(int, int, ...);
inline auto fcntl_f = reinterpret_cast<fcntl_func*>(dlsym(RTLD_NEXT, "fcntl"));

using ioctl_func = int(int, int, ...);
inline auto ioctl_f = reinterpret_cast<ioctl_func*>(dlsym(RTLD_NEXT, "ioctl"));

using getsockopt_func = int(int, int, int, void*, socklen_t*);
inline auto getsockopt_f = OriginalFunction<getsockopt_func>("getsockopt");

using setsockopt_func = int(int, int, int, const void*, socklen_t);
inline auto setsockopt_f = OriginalFunction<setsockopt_func>("setsockopt");
