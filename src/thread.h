#pragma once

#include <functional>
#include <pthread.h>

#include <latch>
#include <string>

namespace sylar {

    class Thread {
    public:
        using thread_func = std::function<void()>;

        explicit Thread(thread_func, std::string = "UNKNOWN");
        // a moved thread cannot be used again
        Thread(Thread&&) noexcept;
        ~Thread();

        void start();
        void join();

        std::string const& getName() const { return name_; }
        pid_t getTid() const { return tid_; }

        static Thread* getCurrentThread();
        static pid_t getCurrentThreadId();

    private:
        static void* run(void*);

        pthread_t thread_{};
        thread_func func_;
        std::string name_;
        pid_t tid_{};
        std::latch latch_{1};

        static inline thread_local Thread* current_thread{};
    };

} // namespace sylar
