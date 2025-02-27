#include "fiber.h"
#include "thread.h"
#include <memory>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>

void foo() {
    spdlog::info("hello");
    sylar::Fiber::yield();
    spdlog::info("bye");
}

void test() {
    spdlog::info("begin test");
    {
        auto fiber = std::make_shared<sylar::Fiber>(foo);
        spdlog::info("before {}", fiber->getId());
        fiber->swapIn();
        spdlog::info("first swapOut {}", fiber->getId());
        fiber->swapIn();
        spdlog::info("second swapOut {}", fiber->getId());
    }
    spdlog::info("leave test");
}

int main() {
    spdlog::set_level(spdlog::level::debug);

    sylar::Fiber::getCurrentFiber();

    std::vector<sylar::Thread> threads;
    for (int i = 0; i < 5; i++) {
        auto thread = sylar::Thread(test, std::format("test_{}", i));
        thread.start();
        threads.emplace_back(std::move(thread));
    }
    for (auto& thread : threads) {
        thread.join();
    }
}
