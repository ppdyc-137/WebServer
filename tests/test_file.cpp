#include "file.h"
#include "io_context.h"
#include "util.h"
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <string>

using namespace sylar;

void test_file() {
    char buf[64];
    while (true) {
        int len = checkRetUring(file_read(stdin_handle(), buf));
        auto read = std::string(buf, static_cast<size_t>(len));
        checkRetUring(file_write(stdout_handle(), read));
    }
}

int main() {
    spdlog::set_level(spdlog::level::debug);

    sylar::IOContext context;
    context.schedule(test_file);
    context.execute();
}
