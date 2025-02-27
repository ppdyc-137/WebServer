#include "thread.h"
#include "util.h"

#include <cstring>
#include <pthread.h>
#include <stdexcept>
#include <sys/syscall.h>
#include <unistd.h>

#include <spdlog/spdlog.h>

namespace sylar {

    Thread::Thread(ThreadFunc func, std::string name) : func_(std::move(func)), name_(std::move(name)) {}
    Thread::Thread(Thread&& other) noexcept
        : thread_(std::exchange(other.thread_, 0)), func_(std::exchange(other.func_, nullptr)),
          name_(std::exchange(other.name_, "")), tid_(std::exchange(other.tid_, 0)) {}

    Thread::~Thread() {
        if (thread_ != 0) {
            pthread_detach(thread_);
        }
    }

    void Thread::start() {
        SYLAR_ASSERT(func_ != nullptr);
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

        pthread_setname_np(thread->thread_, thread->name_.substr(0, PTHREAD_NAME_MAX_LENGTH).c_str());
        spdlog::debug("Thread::run: name: {}, tid: {}", thread->name_, thread->tid_);

        thread->latch_.count_down();

        auto func = thread->func_;
        func();

        return nullptr;
    }

    std::string Thread::getCurrentThreadName() {
        auto thread = pthread_self();
        char buf[PTHREAD_NAME_MAX_LENGTH + 1];
        pthread_getname_np(thread, buf, PTHREAD_NAME_MAX_LENGTH + 1);
        return buf;
    }
    pid_t Thread::getCurrentThreadId() { return static_cast<pid_t>(::syscall(SYS_gettid)); }

} // namespace sylar
