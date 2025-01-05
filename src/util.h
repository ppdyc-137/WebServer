#pragma once

#include <cxxabi.h>
#include <memory>
#include <string>

namespace sylar {

template <typename T>
inline std::string typenameDemangle() {
    int status{};
    const auto *name = typeid(T).name();
    std::unique_ptr<char, void (*)(void*)> res{abi::__cxa_demangle(name, nullptr, nullptr, &status), std::free};

    return (status == 0) ? res.get() : name;
}

}  // namespace sylar
