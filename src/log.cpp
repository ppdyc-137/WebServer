#include "detail/fiber.h"
#include "processor.h"

#include <format>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>

#include "spdlog/pattern_formatter.h"

namespace async {

    class ProcessorFiberFlag : public spdlog::custom_flag_formatter {
    public:
        void format(const spdlog::details::log_msg& /*msg*/, const std::tm& /*tm_time*/,
                    spdlog::memory_buf_t& dest) override {
            std::string text{};
            if (Processor::getProcessor()) {
                auto processor_id = Processor::getProcessorID();
                uint64_t fiber_id{};
                if (auto* fiber = Fiber::getCurrentFiber()) {
                    fiber_id = fiber->getId();
                }
                text = std::format("{} {}", processor_id, fiber_id);
            } else {
                text += "N";
            }
            dest.append(text.data(), text.data() + text.size());
        }

        std::unique_ptr<custom_flag_formatter> clone() const override {
            return spdlog::details::make_unique<ProcessorFiberFlag>();
        }
    };

    namespace {
        struct LogIniter {
            LogIniter() {
                auto formatter = std::make_unique<spdlog::pattern_formatter>();
                formatter->add_flag<ProcessorFiberFlag>('*').set_pattern("[%Y-%m-%d %H:%M:%S:%e] [%*] [%^%l%$] %v");
                spdlog::set_formatter(std::move(formatter));
            }
        } g_log_initer;
    } // namespace

} // namespace async
