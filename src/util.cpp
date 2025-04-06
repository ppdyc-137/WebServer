#include "util.h"
#include "io_context.h"

#include <spdlog/spdlog.h>
#include <thread>

namespace sylar {
    void schedSetThreadAffinity(size_t cpu) {
        cpu_set_t cpu_set;
        CPU_ZERO(&cpu_set);
        CPU_SET(cpu, &cpu_set);
        checkRet(sched_setaffinity(gettid(), sizeof(cpu_set_t), &cpu_set));
    }

    int checkRet(int ret, std::initializer_list<int> ignored, std::source_location location) {
        if (ret < 0) [[unlikely]] {
            for (auto i : ignored) {
                if (ret == i) {
                    return ret;
                }
            }
            spdlog::error("{}: ({}:{}) {} {}", location.file_name(), location.line(), location.column(),
                          location.function_name(), errno);
            throw std::system_error(errno, std::system_category());
        }
        return ret;
    }

    int checkRetUring(int ret, std::initializer_list<int> ignored, std::source_location location) {
        if (ret < 0) [[unlikely]] {
            for (auto i : ignored) {
                if (ret == -i) {
                    return ret;
                }
            }
            spdlog::error("{}: ({}:{}) {} {}", location.file_name(), location.line(), location.column(),
                          location.function_name(), -ret);
            throw std::system_error(-ret, std::system_category());
        }
        return ret;
    }

    void assertThat(bool res, const char* msg, std::source_location location) {
        if (!res) [[unlikely]] {
            spdlog::error("{}: ({}:{}) {}", location.file_name(), location.line(), location.column(),
                          location.function_name());
            throw std::runtime_error(msg ? msg : "assert");
        }
    }

    void sleepFor(std::chrono::system_clock::duration duration) {
        auto* context = IOContext::getCurrentContext();
        if (context) {
            context->addTimer(duration, [fiber = Fiber::getCurrentFiber()]() { IOContext::spawn(fiber); });
            Fiber::yield();
        } else {
            std::this_thread::sleep_for(duration);
        }
    }

} // namespace sylar
