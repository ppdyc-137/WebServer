#pragma once

#include <cerrno>
#include <cxxabi.h>
#include <execinfo.h>
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

    std::vector<std::string> backTrace(int size);
    std::string backTraceToString(int size, const char* prefix = "");

    std::string strevent(uint32_t events);

    uint64_t getCurrentTimeMS();
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
