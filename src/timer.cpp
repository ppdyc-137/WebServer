#include "timer.h"
#include "util.h"
#include <functional>
#include <memory>
#include <utility>

namespace sylar {

    bool Timer::Comparator::operator()(const std::shared_ptr<Timer>& lhs, const std::shared_ptr<Timer>& rhs) const {
        SYLAR_ASSERT(lhs && rhs);

        return lhs->next_trigger_time_ <= rhs->next_trigger_time_;
    }

    Timer::Timer(uint64_t period, std::function<void()> cb, bool recurring, TimerManager* manager)
        : recurring_(recurring), period_(period), next_trigger_time_(getCurrentTimeMS() + period_), cb_(std::move(cb)),
          manager_(manager) {}

    bool Timer::cancel() {
        WriteLockGuard lock(manager_->mutex_);
        auto it = manager_->timers_.find(shared_from_this());
        if (it != manager_->timers_.end()) {
            manager_->timers_.erase(it);
            return true;
        }
        return false;
    }

    bool Timer::refresh() {
        WriteLockGuard lock(manager_->mutex_);
        auto ptr = shared_from_this();
        auto it = manager_->timers_.find(ptr);
        if (it == manager_->timers_.end()) {
            return false;
        }
        manager_->timers_.erase(it);
        next_trigger_time_ = getCurrentTimeMS() + period_;
        manager_->timers_.insert(ptr);
        return true;
    }

    bool Timer::reset(uint64_t ms) {
        if (ms == period_) {
            return true;
        }
        WriteLockGuard lock(manager_->mutex_);
        auto ptr = shared_from_this();
        auto it = manager_->timers_.find(ptr);
        if (it == manager_->timers_.end()) {
            return false;
        }
        manager_->timers_.erase(it);
        period_ = ms;
        next_trigger_time_ = getCurrentTimeMS() + period_;
        it = manager_->timers_.insert(ptr).first;
        if (it == manager_->timers_.begin()) {
            manager_->timerInsertTickle();
        }
        return true;
    }

    std::shared_ptr<Timer> TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring) {
        auto timer = std::shared_ptr<Timer>(new Timer(ms, std::move(cb), recurring, this));
        WriteLockGuard lock(mutex_);
        auto it = timers_.insert(timer).first;
        if (it == timers_.begin()) {
            timerInsertTickle();
        }
        return timer;
    }

    std::shared_ptr<Timer> TimerManager::addCondtionTimer(uint64_t ms, std::function<void()> cb,
                                                          std::weak_ptr<void> cond, bool recurring) {
        return addTimer(
            ms,
            [cond = std::move(cond), cb = std::move(cb)]() {
                if (cond.lock()) {
                    cb();
                }
            },
            recurring);
    }

    uint64_t TimerManager::getNextTriggerTimeLast() {
        ReadLockGuard lock(mutex_);
        if (timers_.empty()) {
            return ~0ULL;
        }

        const auto& timer = *timers_.begin();
        auto now = getCurrentTimeMS();
        if (now > timer->next_trigger_time_) {
            return 0;
        }
        return timer->next_trigger_time_ - now;
    }

    bool TimerManager::hasTimer() {
        ReadLockGuard lock(mutex_);
        return !timers_.empty();
    }

    std::vector<std::function<void()>> TimerManager::getExpiredCallBacks() {
        WriteLockGuard lock(mutex_);
        if (timers_.empty()) {
            return {};
        }
        auto now_timer = std::shared_ptr<Timer>(new Timer(0, nullptr, false, nullptr));
        auto it = timers_.upper_bound(now_timer);
        std::vector<std::shared_ptr<Timer>> expired_timers(timers_.begin(), it);
        timers_.erase(timers_.begin(), it);

        std::vector<std::function<void()>> cbs;
        cbs.reserve(expired_timers.size());
        for (auto& timer : expired_timers) {
            cbs.push_back(timer->cb_);
            if (timer->recurring_) {
                timer->next_trigger_time_ = getCurrentTimeMS() + timer->period_;
                timers_.insert(timer);
            }
        }
        return cbs;
    }

} // namespace sylar
