#pragma once

#include <pthread.h>

namespace sylar {

class Mutex {
public:
    Mutex() = default;
    Mutex(Mutex const&) = delete;
    Mutex& operator=(Mutex const&) = delete;

    void lock() { pthread_mutex_lock(&mutex_); }
    void unlock() { pthread_mutex_unlock(&mutex_); }

private:
    pthread_mutex_t mutex_;
};

template <typename T>
    requires(T{}.lock() && T{}.unlock())
class LockGuard {
public:
    explicit LockGuard(T& mutex) : mutex_(mutex) { mutex_.lock(); }
    ~LockGuard() { mutex_.unlock(); }

    LockGuard(LockGuard const&) = delete;
    LockGuard& operator=(LockGuard const&) = delete;

private:
    T& mutex_;
};

} // namespace sylar
