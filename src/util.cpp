#include "util.h"

#include <spdlog/spdlog.h>
#include <stdexcept>

namespace sylar {
    void schedSetThreadAffinity(size_t cpu) {
        cpu_set_t cpu_set;
        CPU_ZERO(&cpu_set);
        CPU_SET(cpu, &cpu_set);
        checkRet(sched_setaffinity(gettid(), sizeof(cpu_set_t), &cpu_set));
    }

    int checkRet(int ret, std::source_location location) {
        if (ret < 0) [[unlikely]] {
            spdlog::error("{}: ({}:{}) {}", location.file_name(), location.line(), location.column(),
                          location.function_name());
            throw std::system_error(errno, std::system_category());
        }
        return ret;
    }
    int checkRetUring(int ret, std::source_location location) {
        if (ret < 0) [[unlikely]] {
            spdlog::error("{}: ({}:{}) {}", location.file_name(), location.line(), location.column(),
                          location.function_name());
            throw std::system_error(-ret, std::system_category());
        }
        return ret;
    }
    void myAssert(bool res, std::source_location location) {
        if (!res) [[unlikely]] {
            spdlog::error("{}: ({}:{}) {}", location.file_name(), location.line(), location.column(),
                          location.function_name());
            throw std::runtime_error("assert");
        }
    }

} // namespace sylar
