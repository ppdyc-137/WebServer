#include "synchronization/mutex.h"
#include "io_context.h"
#include "util.h"

#include <chrono>

#include <spdlog/common.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/spdlog.h>

using namespace sylar;

void test_cond() {
    ConditionVariable cond;
    Mutex mutex;
    IOContext::spawn([&]() {
        sleepFor(std::chrono::seconds(3));
        cond.notify_one();
    });
    mutex.lock();
    spdlog::info("wait");
    cond.wait(mutex);
    spdlog::info("wake");
    mutex.unlock();
}

Semaphore sem(2);
void consumer(int sec) {
    sem.acquire();
    spdlog::info("{} working...", sec);
    sleepFor(std::chrono::seconds(sec));
    spdlog::info("{} done...", sec);
    sem.release();
};

void test_sem() {
    IOContext::spawn([]() { consumer(3); });
    IOContext::spawn([]() { consumer(5); });
    IOContext::spawn([]() { consumer(4); });
    IOContext::spawn([]() { consumer(2); });
}

void test_mutex() {
    Mutex mutex;
    int sum{};

    auto run = [&](int) {
        IOContext context;
        context.schedule([&]() {
            for (int i = 0; i < 1000000; i++) {
                mutex.lock();
                sum++;
                mutex.unlock();
            }
        });
        context.execute();
    };

    std::thread t1(run, 0), t2(run, 1);
    t1.join();
    t2.join();
    spdlog::info("sum: {}", sum);
}

int main() {
    // spdlog::set_level(spdlog::level::debug);
    IOContext context;
    context.schedule(test_mutex);
    context.execute();
}
