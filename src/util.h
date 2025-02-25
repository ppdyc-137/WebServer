#pragma once

#include <cerrno>
#include <cstddef>
#include <cxxabi.h>
#include <execinfo.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
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

} // namespace sylar
