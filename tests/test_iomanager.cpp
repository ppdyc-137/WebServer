#include "fiber.h"
#include "iomanager.h"

#include <arpa/inet.h>
#include <chrono>
#include <memory>
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
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "1.1.1.1", &addr.sin_addr.s_addr);

    auto read_cb = [sock]() {
        char buf[1024];
        while (true) {
            auto ret = read(sock, buf, sizeof(buf));
            if (ret > 0) {
                spdlog::info("read:\n{}", buf);
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
            auto buf = "GET / HTTP/1.1\r\n\r\n";
            write(sock, buf, strlen(buf) + 1);
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

void test_timer(uint64_t ms, int counts) {
    static std::shared_ptr<sylar::Timer> timer;
    auto callback = [ms, counts]() {
        static int count = 1;
        spdlog::info("tick {}/{} ({}ms)", count++, counts, ms);
        if (count == counts + 1) {
            timer->cancel();
        }
    };
    timer = sylar::IOManager::getCurrentScheduler()->addTimer(ms, callback, true);
}

int main() {
    spdlog::set_level(spdlog::level::debug);

    sylar::IOManager scheduler(4, "Scheduler");
    scheduler.start();

    scheduler.schedule(test_socket);
    // scheduler.schedule([]() {test_timer(1000, 5);});
    // scheduler.schedule(foo, 3);

    // std::this_thread::sleep_for(std::chrono::seconds(100));
}
