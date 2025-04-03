#include "util.h"
#include <bits/types/struct_timeval.h>
#include <sys/select.h>

namespace sylar {
    std::vector<std::string> backTrace(int size) {
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

    std::string backTraceToString(int size, const char* prefix) {
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

    std::string strevent(uint32_t events) {
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

    constexpr uint64_t K = 1000;
    uint64_t getCurrentTimeMS() {
        timeval tp{};
        gettimeofday(&tp, nullptr);
        return (static_cast<uint64_t>(tp.tv_sec) * K) + (static_cast<uint64_t>(tp.tv_usec) / K);
    }

    void schedSetThreadAffinity(size_t cpu) {
        cpu_set_t cpu_set;
        CPU_ZERO(&cpu_set);
        CPU_SET(cpu, &cpu_set);
        SYLAR_ASSERT2(sched_setaffinity(gettid(), sizeof(cpu_set_t), &cpu_set) >= 0, strerror(errno));
    }

} // namespace sylar
