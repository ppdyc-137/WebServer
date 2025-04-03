#pragma once

#include <source_location>

namespace sylar {
    void schedSetThreadAffinity(std::size_t cpu);

    int checkRet(int ret, std::source_location location = std::source_location::current());
    int checkRetUring(int ret, std::source_location location = std::source_location::current());
    void myAssert(bool res, std::source_location location = std::source_location::current());

} // namespace sylar
