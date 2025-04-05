#include "io_context.h"
#include "file/socket.h"

#include <functional>
#include <mutex>

#include <arpa/inet.h>
#include <sys/socket.h>

#include <spdlog/spdlog.h>

using namespace sylar;

const std::string response = "HTTP/1.1 200 OK\r\n"
                             "Content-Type: text/html\r\n"
                             "Content-Length: 13\r\n"
                             "\r\n"
                             "Hello, world!";

void handle(int fd) {
    auto sock = SocketHandle(fd);
    char buf[256];
    while (true) {
        auto ret = socket_read(sock, buf);
        if (ret <= 0) {
            spdlog::debug("{} disconnect", sock.fileNo());
            break;
        }
        auto read = std::string(buf, static_cast<size_t>(ret));
        spdlog::debug("{} read {}: {}", sock.fileNo(), ret, read);
        socket_write(sock, response);
    }
    file_close(std::move(sock));
}

struct Worker {
    using Func = std::function<void()>;
    std::jthread thread;
    std::condition_variable cond;
    std::mutex mutex;
    std::queue<Func> tasks;

    void spawn(Func task) {
        std::lock_guard lock(mutex);
        tasks.push(task);
        cond.notify_one();
    }

    void start(size_t i) {
        thread = std::jthread([this, i]() {
            IOContext manager;
            schedSetThreadAffinity(i);
            while (true) [[likely]] {
                while (manager.runOnce()) {
                    std::unique_lock lck(mutex);
                    if (!tasks.empty()) {
                        break;
                    }
                }

                std::unique_lock lck(mutex);
                cond.wait(lck, [this] { return !tasks.empty(); });
                while (!tasks.empty()) {
                    manager.schedule(tasks.front());
                    tasks.pop();
                }
            }
        });
    }
};

void test_socket() {
    spdlog::info("test_socket");

    auto sock = socket_listen(*AddressResolver().host("127.0.0.1").port(8080).resolve_one(), SOMAXCONN);

    std::vector<Worker> workers(std::thread::hardware_concurrency());
    for (std::size_t i = 0; i < workers.size(); ++i) {
        workers[i].start(i);
    }

    spdlog::info("Listening on port 8080...");
    std::size_t i = 0;
    while (true) {
        auto client_sock = socket_accept(sock).releaseFile();
        workers[i].spawn([client_sock]() { handle(client_sock); });
        ++i;
        if (i >= workers.size()) {
            i = 0;
        }
    }
}

int main() {
    // spdlog::set_level(spdlog::level::debug);
    sylar::IOContext scheduler;
    scheduler.schedule(test_socket);
    scheduler.execute();
}
