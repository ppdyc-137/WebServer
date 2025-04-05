#include "timer.h"

#include <functional>
#include <memory>
#include <optional>
#include <print>
#include <spdlog/spdlog.h>
#include <utility>

namespace sylar {
    TimerManager::Timer::Timer(std::chrono::system_clock::duration period, std::function<void()> cb, bool recurring,
                               TimerManager* manager)
        : recurring_(recurring), period_(period), next_trigger_time_(std::chrono::system_clock::now() + period),
          cb_(std::move(cb)), manager_(manager) {}

    bool TimerManager::Timer::cancel() {
        if (expired_) {
            return false;
        }
        manager_->timers_.erase(shared_from_this());
        return true;
    }

    std::shared_ptr<TimerManager::Timer> TimerManager::addTimer(std::chrono::system_clock::duration period,
                                                                std::function<void()> cb, bool recurring) {
        auto timer = std::make_shared<TimerManager::Timer>(period, std::move(cb), recurring, this);
        timers_.insert(timer);
        return timer;
    }

    std::optional<std::chrono::system_clock::duration> TimerManager::getNextTriggerDuration() {
        if (timers_.empty()) {
            return std::nullopt;
        }
        auto now = std::chrono::system_clock::now();
        auto timer = *timers_.begin();
        if (timer->next_trigger_time_ > now) {
            return timer->next_trigger_time_ - now;
        }
        return std::chrono::seconds(0);
    }

    std::vector<TimerManager::Func> TimerManager::getExpiredCallBacks() {
        std::vector<Func> cbs;
        while (!timers_.empty()) {
            auto now = std::chrono::system_clock::now();
            auto timer_it = timers_.begin();
            const auto& timer = *timer_it;
            if (timer->next_trigger_time_ > now) {
                break;
            }
            cbs.push_back(timer->cb_);
            timers_.erase(timer_it);
            if (timer->recurring_) {
                timer->next_trigger_time_ = now + timer->period_;
                timers_.insert(timer);
            } else {
                timer->expired_ = true;
            }
        }
        return cbs;
    }

} // namespace sylar
