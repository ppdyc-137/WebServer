#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <set>

namespace sylar {
    class TimerManager {
        using Func = std::function<void()>;

    private:
        class Timer : public std::enable_shared_from_this<Timer> {
            friend class TimerManager;

        public:
            Timer(std::chrono::system_clock::duration period, std::function<void()> cb, bool recurring,
                  TimerManager* manager);

            bool cancel();

        private:
            bool expired_{false};
            bool recurring_{};
            std::chrono::system_clock::duration period_;
            std::chrono::system_clock::time_point next_trigger_time_;
            std::function<void()> cb_;
            TimerManager* manager_;

            struct Comparator {
                bool operator()(const std::shared_ptr<Timer>& lhs, const std::shared_ptr<Timer>& rhs) const {
                    return lhs->next_trigger_time_ <= rhs->next_trigger_time_;
                }
            };
        };

    public:
        std::shared_ptr<Timer> addTimer(std::chrono::system_clock::duration period, std::function<void()> cb,
                                        bool recurring = false);

        bool hasTimer() { return timers_.size() != 0; }

    protected:
        std::optional<std::chrono::system_clock::duration> getNextTriggerDuration();
        std::vector<Func> getExpiredCallBacks();
        std::set<std::shared_ptr<Timer>, Timer::Comparator> timers_;
    };

} // namespace sylar
