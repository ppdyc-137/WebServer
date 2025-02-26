#include "fiber.h"
#include "thread.h"
#include <memory>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>

#include "spdlog/pattern_formatter.h"

class my_formatter_flag : public spdlog::custom_flag_formatter {
public:
    void format(const spdlog::details::log_msg&, const std::tm&, spdlog::memory_buf_t& dest) override {
        std::string some_txt = std::format("{} {}", sylar::Thread::getCurrentThreadId(), sylar::Fiber::getCurrentFiberId());
        dest.append(some_txt.data(), some_txt.data() + some_txt.size());
    }

    std::unique_ptr<custom_flag_formatter> clone() const override {
        return spdlog::details::make_unique<my_formatter_flag>();
    }
};

void foo() {
    spdlog::info("hello");
    sylar::Fiber::yield();
    spdlog::info("bye");
}

void test() {
    spdlog::info("begin test");
    {
        auto fiber = std::make_shared<sylar::Fiber>(foo);
        spdlog::info("before {}", fiber->getId());
        fiber->swapIn();
        spdlog::info("first swapOut {}", fiber->getId());
        fiber->swapIn();
        spdlog::info("second swapOut {}", fiber->getId());
    }
    spdlog::info("leave test");
}

int main() {
    {
        auto formatter = std::make_unique<spdlog::pattern_formatter>();
        formatter->add_flag<my_formatter_flag>('*').set_pattern("[%Y-%m-%d %H:%M:%S:%e] [%*] [%^%l%$] %v");
        spdlog::set_formatter(std::move(formatter));
        spdlog::set_level(spdlog::level::debug);
    }

    sylar::Fiber::getCurrentFiber();

    std::vector<std::shared_ptr<sylar::Thread>> threads;
    for (int i = 0; i < 5; i++) {
        auto thread = std::make_shared<sylar::Thread>(test);
        thread->start();
        threads.emplace_back(std::move(thread));
    }
    for (auto& thread : threads) {
        thread->join();
    }
}
