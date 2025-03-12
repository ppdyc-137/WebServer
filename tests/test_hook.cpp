#include "iomanager.h"

#include <spdlog/spdlog.h>

void foo() {
    for (int i = 0; i < 3; i++) {
        spdlog::info("hello {}", i);
        sleep(1);
    }
}
int main() {
    spdlog::set_level(spdlog::level::debug);

    sylar::IOManager scheduler(1, "Scheduler");
    scheduler.start();

    scheduler.schedule(foo, 3);
}
