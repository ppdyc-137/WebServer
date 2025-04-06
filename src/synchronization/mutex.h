#pragma once

#include "detail/fiber.h"
#include "futex.h"

#include <atomic>
#include <cstdint>
#include <spdlog/spdlog.h>

namespace sylar {
    struct Mutex {
    public:
        bool try_lock() {
            auto expected = UNLOCKED;
            return futex_.compare_exchange_strong(expected, LOCKED, std::memory_order_acquire,
                                                  std::memory_order_relaxed);
        }

        void lock() {
            auto state = spin();
            // Before go into contention state, try to grab the lock.
            if (state == UNLOCKED &&
                futex_.compare_exchange_strong(state, LOCKED, std::memory_order_acquire, std::memory_order_relaxed)) {
                return;
            }
            while (true) {
                if (state != IN_CONTENTION && futex_.exchange(IN_CONTENTION, std::memory_order_acquire) == UNLOCKED) {
                    return;
                }
                futex_.wait(IN_CONTENTION);
                state = spin();
            }
        }

        void unlock() {
            auto prev = futex_.exchange(UNLOCKED, std::memory_order_release);
            if (prev == IN_CONTENTION) {
                futex_.notify_one();
            }
        }

    private:
        uint32_t spin(unsigned count = DEFAULT_SPIN_COUNT) {
            while (true) {
                auto result = futex_.load(std::memory_order_relaxed);
                if (result == UNLOCKED || count == 0) {
                    return result;
                }
                Fiber::yield(Fiber::READY);
                count--;
            }
        }
        static constexpr uint32_t UNLOCKED = 0b00;
        static constexpr uint32_t LOCKED = 0b01;
        static constexpr uint32_t IN_CONTENTION = 0b10;

        static constexpr unsigned int DEFAULT_SPIN_COUNT = 100;

        Futex futex_;
    };

    struct ConditionVariable {
    public:
        void wait(Mutex& mutex) {
            mutex.unlock();
            std::uint32_t old = futex_.load(std::memory_order_relaxed);
            while (true) {
                futex_.wait(old);
                if (futex_.load(std::memory_order_acquire) != old) {
                    break;
                }
            }
            mutex.lock();
        }

        void notify_one() {
            futex_.fetch_add(1, std::memory_order_release);
            futex_.notify_one();
        }

        void notify_all() {
            futex_.fetch_add(1, std::memory_order_release);
            futex_.notify_all();
        }

    private:
        Futex futex_;
    };

    struct Semaphore {
    public:
        explicit Semaphore(uint32_t count) : counter_(count) {}
        std::uint32_t count() const noexcept { return counter_.load(std::memory_order_relaxed); }

        void acquire() {
            std::uint32_t count = counter_.load(std::memory_order_relaxed);
            while (true) {
                while (count == 0) {
                    counter_.wait(count);
                    count = counter_.load(std::memory_order_relaxed);
                }
                if (counter_.compare_exchange_weak(count, count - 1, std::memory_order_acq_rel,
                                                   std::memory_order_relaxed)) {
                    break;
                }
            }
        }

        void release() {
            auto old = counter_.fetch_add(1, std::memory_order_release);
            if (old == 0) {
                counter_.notify_one();
            }
        }

    private:
        Futex counter_;
    };

} // namespace sylar
