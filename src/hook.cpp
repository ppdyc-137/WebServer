#include "hook.h"
#include "iomanager.h"

namespace {
    thread_local bool hook_enable = false;
}

namespace sylar {
    bool isHookEnable() { return hook_enable; }
    void setHookEnable(bool enable) { hook_enable = enable; }
} // namespace sylar

constexpr uint64_t K = 1000;
unsigned int sleep(unsigned int seconds) {
    if (!hook_enable) {
        return sleep_f(seconds);
    }
    auto fiber = sylar::Fiber::getCurrentFiber();
    sylar::IOManager::getCurrentScheduler()->addTimer(
        seconds * K, [fiber = std::move(fiber)]() { sylar::IOManager::getCurrentScheduler()->schedule(fiber); }, false);
    sylar::Fiber::yield(sylar::Fiber::HOLD);
    return 0;
}
