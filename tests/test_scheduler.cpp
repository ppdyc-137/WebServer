#include "fiber.h"
#include "scheduler.h"

#include <chrono>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <thread>

void foo() {
    std::size_t i{};
    while (true) {
        spdlog::info("hello {}", i++);
        sylar::Fiber::yield();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main() {
    spdlog::set_level(spdlog::level::debug);

    sylar::Scheduler scheduler(2, "Scheduler");
    scheduler.start();
    scheduler.schedule(foo, 2);
}
