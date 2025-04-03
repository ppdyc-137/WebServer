#include "iomanager.h"
#include "util.h"

#include <arpa/inet.h>
#include <functional>
#include <mutex>
#include <sys/socket.h>

#include <spdlog/spdlog.h>

using namespace sylar;

const std::string response = "HTTP/1.1 200 OK\r\n"
                             "Content-Type: text/html\r\n"
                             "Content-Length: 13\r\n"
                             "\r\n"
                             "Hello, world!";

void handle(int sock) {
    thread_local static char buf[1024];
    while (true) {
        {
            auto ret = UringOp().prep_read(sock, buf, sizeof(buf), 0).res_;
            if (ret <= 0) {
                break;
            }
            spdlog::debug("read {}:\n{}", ret, buf);
        }
        {
            UringOp().prep_write(sock, response.c_str(), static_cast<unsigned int>(response.size()), 0);
        }
    }

    UringOp().prep_close(sock);
}

struct Worker {
    using Task = std::function<void()>;
    std::jthread th;
    std::condition_variable cv;
    std::mutex mtx;
    std::queue<Task> q;

    void spawn(Task task) {
        std::lock_guard lock(mtx);
        q.push(task);
        cv.notify_one();
    }

    void start(size_t i) {
        th = std::jthread([this, i]() {
            IOManager manager;
            schedSetThreadAffinity(i);
            while (true) [[likely]] {
                while (manager.runOnce()) {
                    std::unique_lock lck(mtx);
                    if (!q.empty()) {
                        break;
                    }
                }

                std::unique_lock lck(mtx);
                cv.wait(lck, [this] { return !q.empty(); });
                while (!q.empty()) {
                    manager.schedule(q.front());
                    q.pop();
                }
            }
        });
    }
};

void test_socket() {
    spdlog::debug("test_socket");
    int sock = UringOp().prep_socket(AF_INET, SOCK_STREAM, 0, 0).res_;
    if (sock < 0) {
        spdlog::error("socket error: {}", std::strerror(errno));
        return;
    }
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);

    {
        auto ret = bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        if (ret < 0) {
            spdlog::error("bind error: {}", std::strerror(errno));
            return;
        }
    }

    {
        auto ret = listen(sock, SOMAXCONN);
        if (ret < 0) {
            spdlog::error("listen error: {}", std::strerror(errno));
            return;
        }
    }

    std::vector<Worker> workers(std::thread::hardware_concurrency());

    for (std::size_t i = 0; i < workers.size(); ++i) {
        workers[i].start(i);
    }

    spdlog::info("Listening on port 8080...");
    std::size_t i = 0;
    while (true) {
        sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_sock = UringOp().prep_accept(sock, reinterpret_cast<sockaddr*>(&client_addr), &addr_len, 0).res_;
        if (client_sock < 0) {
            spdlog::error("accept error: {}", std::strerror(errno));
            break;
        }
        workers[i].spawn([client_sock]() { handle(client_sock); });
        ++i;
        if (i >= workers.size()) {
            i = 0;
        }
    }
}

int main() {
    // spdlog::set_level(spdlog::level::debug);
    sylar::IOManager scheduler;
    scheduler.schedule(test_socket);
    scheduler.execute();
}
