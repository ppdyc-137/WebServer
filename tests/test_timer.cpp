#include "io_context.h"
#include "util.h"

#include <chrono>
#include <spdlog/spdlog.h>

using namespace sylar;

void sleep(int sec) {
    while (true) {
        sleepFor(std::chrono::seconds(sec));
        spdlog::info("sleeped {}s", sec);
    }
}

void test_timer() {
    IOContext::spawn([]() { sleep(1); });
    IOContext::spawn([]() { sleep(2); });
}

int main() {
    spdlog::set_level(spdlog::level::debug);
    sylar::IOContext scheduler(2);
    scheduler.spawn(test_timer);
    scheduler.execute();
}
