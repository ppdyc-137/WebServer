#pragma once

#include <chrono>
#include <liburing.h>
#include <source_location>

namespace sylar {
    void schedSetThreadAffinity(std::size_t cpu);

    template <class Rep, class Period>
    struct __kernel_timespec durationToKernelTimespec(std::chrono::duration<Rep, Period> dur) {
        struct __kernel_timespec ts{};
        auto secs = std::chrono::duration_cast<std::chrono::seconds>(dur);
        auto nsecs = std::chrono::duration_cast<std::chrono::nanoseconds>(dur - secs);
        ts.tv_sec = static_cast<__kernel_time64_t>(secs.count());
        ts.tv_nsec = static_cast<__kernel_time64_t>(nsecs.count());
        return ts;
    }

    void sleepFor(std::chrono::system_clock::duration duration);

    int checkRet(int ret, std::source_location location = std::source_location::current());
    int checkRetUring(int ret, std::source_location location = std::source_location::current());
    void myAssert(bool res, std::source_location location = std::source_location::current());

} // namespace sylar
