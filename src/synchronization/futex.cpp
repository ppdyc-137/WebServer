#include "futex.h"

#include "uring_op.h"
#include <sys/syscall.h>
#include <unistd.h>

namespace sylar {
    constexpr unsigned int FUTEX_FLAGS = FUTEX2_SIZE_U32 | FUTEX2_PRIVATE;

    int futex_wait(std::atomic<uint32_t>* futex, uint32_t val, uint32_t mask) {
        return UringOp().prep_futex_wait(reinterpret_cast<uint32_t*>(futex), val, mask, FUTEX_FLAGS, 0).await();
    }

    int futex_notify(std::atomic<uint32_t>* futex, std::size_t count, uint32_t mask) {
        return UringOp().prep_futex_wake(reinterpret_cast<uint32_t*>(futex), count, mask, FUTEX_FLAGS, 0).await();
    }

    int futex_notify_sync(std::atomic<uint32_t>* futex, std::size_t count, uint32_t mask) {
        auto res = syscall(SYS_futex_wait, reinterpret_cast<uint32_t*>(futex), count, mask, FUTEX_FLAGS, 0);
        return static_cast<int>(res);
    }

} // namespace sylar
