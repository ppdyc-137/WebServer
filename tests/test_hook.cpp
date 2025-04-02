#include "iomanager.h"

#include <arpa/inet.h>
#include <sys/socket.h>

#include <spdlog/spdlog.h>

using namespace sylar;

void handle(int sock) {
    {
        char buf[1024];
        auto ret = UringOp().prep_read(sock, buf, sizeof(buf), 0).res_;
        if (ret < 0) {
            spdlog::error("read error {}", std::strerror(errno));
            goto close;
        }
        spdlog::debug("read {}:\n{}", ret, buf);
    }
    {
        auto buf = "HTTP/1.1 200 OK\r\n"
                   "Content-Type: text/html\r\n"
                   "Content-Length: 13\r\n"
                   "\r\n"
                   "Hello, world!";
        auto ret = UringOp().prep_write(sock, buf, strlen(buf) + 1, 0).res_;
        if (ret < 0) {
            spdlog::error("write error: {}", std::strerror(errno));
            goto close;
        }
    }

close:
    close(sock);
}

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

    spdlog::info("Listening on port 8080...");
    while (true) {
        sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_sock = UringOp().prep_accept(sock, reinterpret_cast<sockaddr*>(&client_addr), &addr_len, 0).res_;
        if (client_sock < 0) {
            spdlog::error("accept error: {}", std::strerror(errno));
            break;
        }

        sylar::IOManager::getCurrentScheduler()->schedule([client_sock]() { handle(client_sock); });
    }
}

int main() {
    // spdlog::set_level(spdlog::level::debug);
    sylar::IOManager scheduler;
    scheduler.schedule(test_socket);
    scheduler.execute();
}
