#pragma once

#include <cerrno>
#include <cstddef>
#include <cxxabi.h>
#include <execinfo.h>
#include <format>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <sys/epoll.h>
#include <vector>

namespace sylar {

    template <typename T>
    inline std::string typenameDemangle() {
        int status{};
        const auto* name = typeid(T).name();
        std::unique_ptr<char, void (*)(void*)> res{abi::__cxa_demangle(name, nullptr, nullptr, &status), std::free};

        return (status == 0) ? res.get() : name;
    }

    inline std::vector<std::string> backTrace(int size) {
        auto buffer = std::vector<void*>(static_cast<size_t>(size));
        auto nptrs = static_cast<size_t>(backtrace(buffer.data(), size));

        char** strings = backtrace_symbols(buffer.data(), size);
        if (strings == nullptr) {
            spdlog::error("backTrace: backtrace_symbols error: {}", std::strerror(errno));
            return {};
        }

        std::vector<std::string> trace(nptrs);
        for (size_t i = 0; i < nptrs; i++) {
            trace[i] = strings[i];
        }
        return trace;
    }
    inline std::string backTraceToString(int size, const char* prefix = "") {
        auto buffer = std::vector<void*>(static_cast<size_t>(size));
        auto nptrs = static_cast<size_t>(backtrace(buffer.data(), size));

        char** strings = backtrace_symbols(buffer.data(), size);
        if (strings == nullptr) {
            spdlog::error("backTrace: backtrace_symbols error: {}", std::strerror(errno));
            return {};
        }

        std::string ret;
        for (size_t i = 0; i < nptrs; i++) {
            ret += std::format("{}{}\n", prefix, strings[i]);
        }
        return ret;
    }

    inline static std::string strevent(uint32_t events) {
        std::stringstream ss;
        if (events & EPOLLIN) {
            ss << "EPOLLIN ";
        }
        if (events & EPOLLOUT) {
            ss << "EPOLLOUT ";
        }
        if (events & EPOLLERR) {
            ss << "EPOLLERR ";
        }
        if (events & EPOLLHUP) {
            ss << "EPOLLHUP ";
        }
        if (events & EPOLLRDHUP) {
            ss << "EPOLLRDHUP ";
        }
        if (events & EPOLLPRI) {
            ss << "EPOLLPRI ";
        }
        if (events & EPOLLET) {
            ss << "EPOLLET ";
        }
        if (events & EPOLLONESHOT) {
            ss << "EPOLLONESHOT ";
        }
        return ss.str();
    }
} // namespace sylar

#define SYLAR_ASSERT(x)                                                                                                \
    if (!(x)) {                                                                                                        \
        spdlog::error("ASSERTION: {}\nbacktrace:\n{}", #x, sylar::backTraceToString(100));                             \
        assert(x);                                                                                                     \
    }

#define SYLAR_ASSERT2(x, msg)                                                                                          \
    if (!(x)) {                                                                                                        \
        spdlog::error("ASSERTION: {}\n{}\nbacktrace:\n{}", #x, msg, sylar::backTraceToString(100, "    "));            \
        assert(x);                                                                                                     \
    }
