#pragma once

#include "detail/fiber.h"
#include "processor.h"
#include "runqueue.h"
#include "util.h"

#include <spdlog/spdlog.h>

namespace async {
    class IOContext {
    public:
        using Func = std::function<void()>;
        using Task = Fiber*;

        explicit IOContext(size_t thread_count = std::thread::hardware_concurrency(), bool hook = false) : hook_(hook) {
            assertThat(instance == nullptr);
            instance = this;
            threads_.resize(thread_count);
            processors_.resize(thread_count);
        }
        ~IOContext() {
            for (auto& thread : threads_) {
                thread.join();
            }
        }

        static IOContext* getInstance() {
            assertThat(instance);
            return instance;
        }

        void execute();

        // spawn a task, like keyword go in golang
        // by default push task into processor's local task queue, if it's full, push the task into gloabl queue
        static void spawn(Func const& func) {
            assertThat(instance);

            auto* processor = Processor::getProcessor();
            if (processor == nullptr) {
                instance->emplaceTask(func);
            } else {
                processor->emplaceTask(func);
            }
        }
        static void spawn(Task task) {
            assertThat(instance);

            auto* processor = Processor::getProcessor();
            if (processor == nullptr) {
                instance->emplaceTask(task);
            } else {
                processor->emplaceTask(task);
            }
        }
        static void spawnAt(size_t id, Func const& func) {
            assertThat(instance);

            auto* processor = instance->processors_[id];
            processor->emplaceTask(func);
        }
        static void spawnAt(size_t id, Task task) {
            assertThat(instance);

            auto* processor = instance->processors_[id];
            processor->emplaceTask(task);
        }

    private:
        friend class Processor;
        size_t stealTasks(uint64_t id, RunQueue& rq);

        void emplaceTask(Func const& func) { emplaceTask(rq_.buildTask(func)); }
        void emplaceTask(Task task);

        bool hook_;

        RunQueue rq_{1024};

        std::vector<std::thread> threads_;
        std::vector<Processor*> processors_;

        static inline IOContext* instance;
    };

} // namespace async
