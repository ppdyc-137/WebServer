#include "thread.h"

#include <cstddef>
#include <format>
#include <iostream>
#include <memory>
#include <vector>

#include <spdlog/spdlog.h>

#include "mutex.h"

int main() {
    spdlog::set_level(spdlog::level::debug);

    std::vector<std::shared_ptr<saylar::Thread>> threads;

    {
        std::size_t counter = 0;
        sylar::Mutex mutex;

        auto worker = [&]() {
            while (true) {
                sylar::LockGuard lock(mutex);
                counter++;
            }
        };

        for (size_t i = 0; i < 5; i++) {
            auto thread = std::make_shared<saylar::Thread>(worker, std::format("thread_{}", i));
            thread->start();
            threads.push_back(std::move(thread));
        }
        for (auto& thread : threads) {
            thread->join();
        }

        std::cout << counter << '\n';
    }

    // {
    //     std::size_t counter = 0;
    //     sylar::RWMutex mutex;
    //
    //     auto writer = [&]() {
    //         while (true) {
    //             sylar::WriteLockGuard lock(mutex);
    //             counter ++;
    //         }
    //     };
    //
    //     auto reader = [&]() {
    //         std::size_t val;
    //         while(true) {
    //             {
    //                 sylar::ReadLockGuard lock(mutex);
    //                 val = counter;
    //             }
    //         }
    //         (void) val;
    //     };
    //
    //     for (size_t i = 0; i < 5; i++) {
    //         auto thread = std::make_shared<saylar::Thread>(reader, std::format("thread_read_{}", i));
    //         thread->start();
    //         threads.push_back(std::move(thread));
    //     }
    //     for (size_t i = 0; i < 5; i++) {
    //         auto thread = std::make_shared<saylar::Thread>(writer, std::format("thread_write_{}", i));
    //         thread->start();
    //         threads.push_back(std::move(thread));
    //     }
    //
    //     for (auto& thread : threads) {
    //         thread->join();
    //     }
    // }
}
