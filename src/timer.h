#pragma once

#include "mutex.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <set>

namespace sylar {
    class TimerManager;
    class Timer : public std::enable_shared_from_this<Timer> {
        friend class TimerManager;

    public:
        bool cancel();
        bool refresh();
        bool reset(uint64_t ms);

    private:
        Timer(uint64_t period, std::function<void()> cb, bool recurring, TimerManager* manager);

        bool recurring_{};
        uint64_t period_{};
        uint64_t next_trigger_time_{};
        std::function<void()> cb_;
        TimerManager* manager_;

        struct Comparator {
            bool operator()(const std::shared_ptr<Timer>& lhs, const std::shared_ptr<Timer>& rhs) const;
        };
    };

    class TimerManager {
        friend class Timer;

    public:
        TimerManager() = default;
        virtual ~TimerManager() = default;

        std::shared_ptr<Timer> addTimer(uint64_t ms, std::function<void()> cb, bool recurring);
        std::shared_ptr<Timer> addCondtionTimer(uint64_t ms, std::function<void()> cb, std::weak_ptr<void> cond,
                                                bool recurring);

        uint64_t getNextTriggerTimeLast();
        bool hasTimer();

        std::vector<std::function<void()>> getExpiredCallBacks();

    private:
        virtual void timerInsertTickle() = 0;

        RWMutex mutex_;
        std::set<std::shared_ptr<Timer>, Timer::Comparator> timers_;
    };
} // namespace sylar
