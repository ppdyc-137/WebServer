#include "io_context.h"
#include "synchronization/mutex.h"
#include "util.h"

#include <chrono>

#include <latch>
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

void test_sem() {
    Semaphore sem(2);
    auto consumer = [&](int sec) {
        sem.acquire();
        spdlog::info("{} working...", sec);
        sleepFor(std::chrono::seconds(sec));
        spdlog::info("{} done...", sec);
        sem.release();
    };
    IOContext::spawn([consumer]() { consumer(3); });
    IOContext::spawn([consumer]() { consumer(5); });
    IOContext::spawn([consumer]() { consumer(4); });
    IOContext::spawn([consumer]() { consumer(2); });
}

// run in main thread
void test_mutex() {
    const ptrdiff_t nr_task = 10;
    {
        std::latch finish(nr_task);

        long sum{};

        auto task = [&]() {
            for (int i = 0; i < 1000000; i++) {
                sum++;
            }
            finish.count_down();
        };
        for (int i = 0; i < nr_task; i++) {
            IOContext::spawn(task);
        }
        finish.wait();

        spdlog::info("sum without mutex: {}", sum);
    }
    {
        std::latch finish(nr_task);

        long sum{};

        Mutex mutex;
        auto task = [&]() {
            for (int i = 0; i < 1000000; i++) {
                mutex.lock();
                sum++;
                mutex.unlock();
            }
            finish.count_down();
        };
        for (int i = 0; i < nr_task; i++) {
            IOContext::spawn(task);
        }
        finish.wait();

        spdlog::info("sum with mutex: {}", sum);
    }
}

int main() {
    // spdlog::set_level(spdlog::level::debug);
    IOContext context;
    context.execute();

    test_mutex();
}
