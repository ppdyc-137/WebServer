#pragma once

#include <pthread.h>

namespace sylar {

    class Mutex {
    public:
        Mutex() { pthread_mutex_init(&mutex_, nullptr); }
        ~Mutex() { pthread_mutex_destroy(&mutex_); }
        Mutex(Mutex const&) = delete;
        Mutex& operator=(Mutex const&) = delete;

        void lock() { pthread_mutex_lock(&mutex_); }
        void unlock() { pthread_mutex_unlock(&mutex_); }

    private:
        pthread_mutex_t mutex_{};
    };

    class SpinLock {
    public:
        SpinLock() { pthread_spin_init(&lock_, 0); }
        ~SpinLock() { pthread_spin_destroy(&lock_); }
        SpinLock(SpinLock const&) = delete;
        SpinLock& operator=(SpinLock const&) = delete;

        void lock() { pthread_spin_lock(&lock_); }
        void unlock() { pthread_spin_unlock(&lock_); }

    private:
        pthread_spinlock_t lock_{};
    };

    class RWMutex {
    public:
        RWMutex() { pthread_rwlock_init(&lock_, nullptr); }
        ~RWMutex() { pthread_rwlock_destroy(&lock_); }
        RWMutex(RWMutex const&) = delete;
        RWMutex& operator=(RWMutex const&) = delete;

        void readLock() { pthread_rwlock_rdlock(&lock_); }
        void writeLock() { pthread_rwlock_wrlock(&lock_); }
        void unlock() { pthread_rwlock_unlock(&lock_); }

    private:
        pthread_rwlock_t lock_{};
    };

    template <typename T>
    concept MutexType = requires(T obj) {
        { obj.lock() };
        { obj.unlock() };
    };

    template <typename T>
    concept RWMutexType = requires(T obj) {
        { obj.readLock() };
        { obj.writeLock() };
        { obj.unlock() };
    };

    template <typename T>
    class LockGuardBase {
    public:
        explicit LockGuardBase(T& mutex) : mutex_(mutex) {}
        virtual ~LockGuardBase() { mutex_.unlock(); }

        LockGuardBase(LockGuardBase const&) = delete;
        LockGuardBase& operator=(LockGuardBase const&) = delete;

    private:
        T& mutex_;
    };

    template <MutexType T>
    class LockGuard : LockGuardBase<T> {
    public:
        explicit LockGuard(T& mutex) : LockGuardBase<T>(mutex) { mutex.lock(); }
    };

    template <RWMutexType T>
    class ReadLockGuard : LockGuardBase<T> {
    public:
        explicit ReadLockGuard(T& mutex) : LockGuardBase<T>(mutex) { mutex.readLock(); }
    };

    template <RWMutexType T>
    class WriteLockGuard : LockGuardBase<T> {
    public:
        explicit WriteLockGuard(T& mutex) : LockGuardBase<T>(mutex) { mutex.writeLock(); }
    };

} // namespace sylar
