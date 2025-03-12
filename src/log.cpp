#include "log.h"
#include "fiber.h"
#include "thread.h"

#include <spdlog/common.h>
#include <spdlog/spdlog.h>

#include "spdlog/pattern_formatter.h"

class ThreadFiberIdFlag : public spdlog::custom_flag_formatter {
public:
    void format(const spdlog::details::log_msg& /*msg*/, const std::tm& /*tm_time*/,
                spdlog::memory_buf_t& dest) override {
        std::string text{};
        text += std::format("{:15} {} {:3}", sylar::Thread::getCurrentThreadName(), sylar::Thread::getCurrentThreadId(), sylar::Fiber::getCurrentFiberId());
        dest.append(text.data(), text.data() + text.size());
    }

    std::unique_ptr<custom_flag_formatter> clone() const override {
        return spdlog::details::make_unique<ThreadFiberIdFlag>();
    }
};

namespace sylar {

    namespace {
        struct LogIniter {
            LogIniter() {
                auto formatter = std::make_unique<spdlog::pattern_formatter>();
                formatter->add_flag<ThreadFiberIdFlag>('*').set_pattern("[%Y-%m-%d %H:%M:%S:%e] [%30*] [%^%-5l%$] %v");
                spdlog::set_formatter(std::move(formatter));
            }
        } g_log_initer;
    } // namespace

} // namespace sylar
