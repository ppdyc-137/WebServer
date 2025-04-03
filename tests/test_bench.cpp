#include "iomanager.h"
#include "net/http.h"
#include "net/socket.h"

#include <csignal>
#include <spdlog/spdlog.h>

const sylar::HttpResponse response_template = {
    .version_ = 0x10,
    .status_ = sylar::HttpStatus::OK,
    .reason_ = "",
    .headers_ = {{"Content-Type", "text/plain"}},
    .cookies_ = {},
    .body_ = "T",
};
auto response_str = response_template.toString();

void handle(std::shared_ptr<sylar::Socket> client_sock) {
    thread_local static char buf[1024];
    while (true) {
        auto ret = client_sock->recv(buf, sizeof(buf));
        if (ret <= 0) {
            return;
        }
        ret = client_sock->send(response_str.c_str(), response_str.size());
    }
}

auto manager = sylar::IOManager(1, "scheduler");
void signal_handler(int) {
    spdlog::info("Fiber Created {}", sylar::Fiber::getFiberCreated());
    spdlog::info("events: {}", manager.getEventsCount());
    spdlog::info("tasks: {}, free: {}", manager.getTaskCount(), manager.getFreeTaskCount());
    spdlog::info("threads: active: {}, idle: {}", manager.getActiveThreadCount(), manager.getIdleThreadCount());
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

    while (true) {
        auto client_sock = server_sock->accept();
        if (!client_sock) {
            break;
        }
        manager.schedule(
            [client_sock = std::move(client_sock)]() { handle(std::move(client_sock)); });
    }
}


int main() {
    signal(SIGINT, signal_handler);

    // spdlog::set_level(spdlog::level::debug);
    manager.start();
    manager.schedule(test_echo_server);
}
