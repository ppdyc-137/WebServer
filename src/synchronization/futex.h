#pragma once

#include "util.h"

#include "linux/futex.h"

#include <atomic>
#include <limits>

namespace sylar {
    int futex_wait(std::atomic<uint32_t>* futex, uint32_t val, uint32_t mask = FUTEX_BITSET_MATCH_ANY);
    int futex_notify(std::atomic<uint32_t>* futex, std::size_t count, uint32_t mask = FUTEX_BITSET_MATCH_ANY);
    int futex_notify_sync(std::atomic<uint32_t>* futex, std::size_t count, uint32_t mask = FUTEX_BITSET_MATCH_ANY);

    constexpr std::size_t FUTEX_NOTIFY_ALL = static_cast<std::size_t>(std::numeric_limits<int>::max());

    class Futex : public std::atomic<uint32_t> {
    public:
        void wait(uint32_t expected, uint32_t mask = FUTEX_BITSET_MATCH_ANY) {
            if (this->load(std::memory_order_relaxed) != expected) {
                return;
            }
            checkRetUring(futex_wait(this, expected, mask), {EAGAIN});
        }
        void notify_one(uint32_t mask = FUTEX_BITSET_MATCH_ANY) { checkRetUring(futex_notify(this, 1, mask)); }
        void notify_all(uint32_t mask = FUTEX_BITSET_MATCH_ANY) {
            checkRetUring(futex_notify(this, FUTEX_NOTIFY_ALL, mask));
        }
    };
} // namespace sylar
