#include "fiber.h"
#include "scheduler.h"

#include <chrono>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <thread>

void foo() {
    for (int i = 0; i < 5; i++) {
        spdlog::info("hello {}", i);
        sylar::Fiber::yield();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main() {
    spdlog::set_level(spdlog::level::debug);

    sylar::Scheduler scheduler(4, "Scheduler");
    scheduler.start();
    scheduler.schedule(foo, 2);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    scheduler.schedule(foo, 2);
}
