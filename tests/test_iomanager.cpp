#include "fiber.h"
#include "iomanager.h"

#include <arpa/inet.h>
#include <chrono>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <sys/epoll.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <thread>
#include <unistd.h>

void foo() {
    for (int i = 0; i < 3; i++) {
        spdlog::info("hello {}", i);
        sylar::Fiber::yield();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void test_socket() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8000);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);

    auto read_cb = [sock]() {
        char buf[256];
        while (true) {
            auto ret = read(sock, buf, sizeof(buf));
            if (ret > 0) {
                spdlog::info("read {}", buf);
                continue;
            }
            if (ret < 0 && errno != EAGAIN) {
                spdlog::error("read error {}", std::strerror(errno));
            }
            break;
        }
        close(sock);
    };
    auto write_cb = [sock, read_cb]() {
        int result;
        socklen_t result_len = sizeof(result);
        getsockopt(sock, SOL_SOCKET, SO_ERROR, &result, &result_len);
        if (result == 0) {
            spdlog::info("connected");
            sylar::IOManager::getCurrentScheduler()->addEvent(sock, EPOLLIN, read_cb);
        } else {
            spdlog::error("connect error: {}", std::strerror(result));
            close(sock);
        }
    };
    auto ret = connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (ret < 0) {
        if (errno == EINPROGRESS) {
            sylar::IOManager::getCurrentScheduler()->addEvent(sock, EPOLLOUT, write_cb);
        } else {
            perror("connect");
            close(sock);
            return;
        }
    }
}

void test_timer() {
    int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timer_fd == -1) {
        perror("timerfd_create");
        return;
    }

    // Set timer to expire after 1 second, then every 1 second
    struct itimerspec new_value;
    new_value.it_value.tv_sec = 1;
    new_value.it_value.tv_nsec = 0;
    new_value.it_interval.tv_sec = 1;
    new_value.it_interval.tv_nsec = 0;

    if (timerfd_settime(timer_fd, 0, &new_value, NULL) == -1) {
        perror("timerfd_settime");
        close(timer_fd);
        return;
    }

    static int count = 1;
    auto callback = [timer_fd]() {
        while (true) {
            spdlog::info("tick {}", count++);
            uint64_t expirations;
            ssize_t s = read(timer_fd, &expirations, sizeof(expirations));
            if (s != sizeof(expirations)) {
                perror("read");
                close(timer_fd);
                return;
            }
            spdlog::info("Timer expired {} times", expirations);
            if (count == 5) {
                close(timer_fd);
            } else {
                sylar::IOManager::getCurrentScheduler()->addEvent(timer_fd, EPOLLIN);
            }
            sylar::Fiber::yield(sylar::Fiber::HOLD);
        }
    };
    sylar::IOManager::getCurrentScheduler()->addEvent(timer_fd, EPOLLIN, callback);
}

int main() {
    spdlog::set_level(spdlog::level::debug);

    sylar::IOManager scheduler(4, "Scheduler");
    scheduler.start();

    scheduler.schedule(test_socket);
    scheduler.schedule(test_timer);
    scheduler.schedule(foo, 2);

    // std::this_thread::sleep_for(std::chrono::seconds(100));
}
