#include "net/http.h"
#include "net/socket.h"
#include "iomanager.h"

#include <spdlog/spdlog.h>

const sylar::HttpResponse response_template = {
    .version_ = 0x11,
    .status_ = sylar::HttpStatus::OK,
    .reason_ = "",
    .headers_ = {
        {"content-type", "text/plain"}
    },
    .cookies_ = {},
    .body_ = "<h1>hello</h1>",
};
auto response_str = response_template.toString();

void handle(std::shared_ptr<sylar::Socket> client_sock) {
    auto addr = client_sock->getPeerAddress()->toString();
    // spdlog::info("accept: {}", addr);

    char buf[1024];
    while (true) {
        auto ret = client_sock->recv(buf, sizeof(buf));
        if (ret < 0) {
            spdlog::error("read error: {} {}", addr, strerror(errno));
            break;
        }
        if (ret == 0) {
            // spdlog::info("{} disconnected", addr);
            break;
        }
        ret = client_sock->send(response_str.c_str(), response_str.size());
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
    if (!server_sock->listen(0)) {
        spdlog::error("listen error: {}", strerror(errno));
        return;
    }

    // spdlog::info("listening: {}", addr.toString());

    server_sock->setRecvTimeout(5000);
    while (true) {
        auto client_sock = server_sock->accept();
        if (!client_sock) {
            break;
        }
        sylar::IOManager::getCurrentScheduler()->schedule([client_sock = std::move(client_sock)]() { handle(std::move(client_sock)); });
    }
}

int main() {
    spdlog::set_level(spdlog::level::debug);
    auto manager = sylar::IOManager(1, "scheduler");
    manager.start();
    manager.schedule(test_echo_server);
}
