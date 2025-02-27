#pragma once

#include <functional>
#include <pthread.h>

#include <latch>
#include <string>

namespace sylar {

    class Thread {
    public:
        using ThreadFunc = std::function<void()>;

        explicit Thread(ThreadFunc, std::string = "UNKNOWN");
        // a moved thread cannot be used again
        Thread(Thread&&) noexcept;
        ~Thread();

        void start();
        void join();

        std::string const& getName() const { return name_; }
        pid_t getTid() const { return tid_; }

        static std::string getCurrentThreadName();
        static pid_t getCurrentThreadId();

    private:
        static void* run(void*);

        pthread_t thread_{};
        ThreadFunc func_;
        std::string name_;
        pid_t tid_{};
        std::latch latch_{1};
    };

} // namespace sylar
