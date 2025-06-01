#include "io_context.h"

#include <chrono>
#include <latch>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <thread>

using namespace sylar;

size_t nr_p = 40;
int count = 240;

void test_worksteal() {
    std::latch finish(count);

    spdlog::info("start {} sleep for 1s with {} Processors", count, nr_p);
    for (int i = 0; i < count; i++) {
        IOContext::spawn([&, i]() {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            spdlog::debug("hello {} from {}", i, Processor::getProcessorID());
            finish.count_down();
        });
    }

    spdlog::info("spawn finish");
    finish.wait();
    spdlog::info("sleep finish");
}

int main() {
    // spdlog::set_level(spdlog::level::debug);

    sylar::IOContext scheduler(nr_p);
    scheduler.execute();

    test_worksteal();
}
