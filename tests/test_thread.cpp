#include "thread.h"

#include <format>
#include <memory>
#include <spdlog/spdlog.h>
#include <vector>
#include <iostream>

int main() {
    spdlog::set_level(spdlog::level::debug);

    std::vector<std::shared_ptr<saylar::Thread>> threads;

    auto func = []() {
        auto* current_thread = saylar::Thread::getCurrentThread();
        std::cout << std::format("name: {}, tid: {}\n", current_thread->getName(), current_thread->getTid());
        while (true) {}
    };

    for (size_t i = 0; i < 5; i++) {
        auto thread = std::make_shared<saylar::Thread>(func, std::format("thread_{}", i));
        thread->start();
        threads.push_back(thread);
    }

    for (auto & thread: threads) {
        thread->join();
    }
}
