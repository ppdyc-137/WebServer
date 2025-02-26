#include "thread.h"

#include <cstring>
#include <pthread.h>
#include <stdexcept>
#include <sys/syscall.h>
#include <unistd.h>

#include <spdlog/spdlog.h>

namespace sylar {

    Thread::Thread(thread_func func, std::string name) : func_(std::move(func)), name_(std::move(name)) {}

    Thread::~Thread() {
        if (thread_ != 0) {
            pthread_detach(thread_);
        }
    }

    void Thread::start() {
        auto ret = pthread_create(&thread_, nullptr, &Thread::run, this);
        if (ret != 0) {
            spdlog::error("Thread::start: pthread_create error: {}", std::strerror(ret));
            thread_ = 0;
            throw std::runtime_error("Thread::start error");
        }
        latch_.wait();
    }

    void Thread::join() {
        if (thread_ != 0) {
            auto ret = pthread_join(thread_, nullptr);
            if (ret != 0) {
                spdlog::error("Thread::start: pthread_create error: {}", std::strerror(ret));
                throw std::runtime_error("Thread::join error");
            }
            thread_ = 0;
        }
    }

    // The thread name is a meaningful C language string, whose length is restricted to 16 characters,  including  the
    // terminating  null byte ('\0').
    constexpr size_t PTHREAD_NAME_MAX_LENGTH = 15;
    void* Thread::run(void* arg) {
        auto* thread = static_cast<Thread*>(arg);
        thread->tid_ = static_cast<pid_t>(::syscall(SYS_gettid));

        spdlog::debug("Thread::run: name: {}, tid: {}", thread->name_, thread->tid_);

        current_thread = thread;
        pthread_setname_np(thread->thread_, thread->name_.substr(0, PTHREAD_NAME_MAX_LENGTH).c_str());

        thread->latch_.count_down();
        thread->func_();

        return nullptr;
    }

    Thread* Thread::getCurrentThread() { return current_thread; }
    pid_t Thread::getCurrentThreadId() { return static_cast<pid_t>(::syscall(SYS_gettid)); }

} // namespace sylar
