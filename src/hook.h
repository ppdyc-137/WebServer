#pragma once

#include <dlfcn.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

namespace sylar {
    bool isHookEnable();
    void setHookEnable(bool enable);
} // namespace sylar

struct Handle {
    Handle() : handle_(dlopen("libc.so.6", RTLD_LAZY)) {
        if (!handle_) {
            throw std::runtime_error(dlerror());
        }
    }
    ~Handle() {
        if (handle_) {
            dlclose(handle_);
        }
    }
    static void* handle() {
        static Handle handle;
        return handle.handle_;
    }
    void* handle_;
};

template <typename Func>
struct OriginalFunction {
    explicit OriginalFunction(const char* name) : func_(reinterpret_cast<Func*>(dlsym(Handle::handle(), name))) {
        if (!func_) {
            throw std::runtime_error(dlerror());
        }
    }

    template <typename... Args>
    auto operator()(Args&&... args) const {
        return func_(std::forward<Args>(args)...);
    }
    Func* func_;
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

using recv_func = ssize_t(int, void*, size_t, int);
inline auto recv_f = OriginalFunction<recv_func>("recv");

using recvfrom_func = ssize_t(int, void*, size_t, int, struct sockaddr*, socklen_t*);
inline auto recvfrom_f = OriginalFunction<recvfrom_func>("recvfrom");

using write_func = ssize_t(int, const void*, size_t);
inline auto write_f = OriginalFunction<write_func>("write");

using send_func = ssize_t(int, const void*, size_t, int);
inline auto send_f = OriginalFunction<send_func>("send");

using sendto_func = ssize_t(int, const void*, size_t, int, const struct sockaddr*, socklen_t);
inline auto sendto_f = OriginalFunction<sendto_func>("sendto");

using close_func = int(int);
inline auto close_f = OriginalFunction<close_func>("close");
