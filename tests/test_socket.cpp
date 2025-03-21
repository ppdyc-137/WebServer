#include "address.h"
#include "iomanager.h"
#include "socket.h"

#include <chrono>
#include <memory>
#include <sys/socket.h>

#include <spdlog/spdlog.h>
#include <thread>
#include <unistd.h>

void handle(std::shared_ptr<sylar::Socket> client_sock) {
    auto addr = client_sock->getPeerAddress()->toString();
    spdlog::info("accept: {}", addr);

    char buf[1024];
    while (true) {
        auto ret = client_sock->recv(buf, sizeof(buf));
        if (ret < 0) {
            spdlog::error("read error: {} {}", addr, strerror(errno));
            break;
        }
        if (ret == 0) {
            spdlog::info("{} disconnected", addr);
            break;
        }
        spdlog::info("{}: {}", addr, buf);
        ret = client_sock->send(buf, strlen(buf) + 1);
        if (ret < 0) {
            spdlog::error("write error: {} {}", addr, strerror(errno));
            break;
        }
    }
}

void test_echo_server() {
    auto server_sock = sylar::Socket::newTcpSocket();
    if (!server_sock) {
        spdlog::error("socker error: {}", strerror(errno));
        return;
    }

    auto addr = sylar::IPV4Address("127.0.0.1", 8080);
    if (!server_sock->bind(addr)) {
        spdlog::error("bind error: {}", strerror(errno));
        return;
    }
    if (!server_sock->listen(10)) {
        spdlog::error("listen error: {}", strerror(errno));
        return;
    }

    spdlog::info("listening: {}", addr.toString());

    while (true) {
        auto client_sock = server_sock->accept();
        if (!client_sock) {
            spdlog::error("accept error: {}", strerror(errno));
            continue;
        }
        sylar::IOManager::getCurrentScheduler()->schedule([client_sock]() { handle(std::move(client_sock)); });
    }
}

int main() {
    spdlog::set_level(spdlog::level::debug);
    auto manager = sylar::IOManager(2, "TcpServer");
    manager.start();
    manager.schedule(test_echo_server);
    std::this_thread::sleep_for(std::chrono::seconds(10));
}
