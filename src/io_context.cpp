#include "io_context.h"

#include <cstdlib>
#include <latch>
#include <liburing.h>
#include <spdlog/spdlog.h>

namespace sylar {
    namespace {
        size_t randamN(size_t N) { return static_cast<size_t>(rand()) % N; }
    } // namespace

    size_t IOContext::stealTasks(uint64_t id, RunQueue& rq) {
        // get tasks from global queue
        auto size = this->rq_.steal(rq, true);
        if (size > 0) {
            return size;
        }
        // steal tasks from other processors
        uint64_t start = randamN(processors_.size());
        for (size_t i = 0; i < processors_.size(); i++) {
            auto pid = (start + i) % processors_.size();
            if (pid == id) {
                continue;
            }
            auto size = processors_[pid]->stealTasks(rq);
            if (size > 0) {
                return size;
            }
        }
        return 0;
    }

    void IOContext::execute() {
        // make sure all processor is initialized
        std::latch init_finish(static_cast<std::ptrdiff_t>(threads_.size()) + 1);

        for (size_t i = 0; i < threads_.size(); ++i) {
            threads_[i] = std::thread([&, i]() {
                Processor processor(i, hook_);
                processors_[i] = &processor;

                init_finish.arrive_and_wait();
                processor.execute();
            });
        }
        init_finish.arrive_and_wait();
    }

} // namespace sylar
