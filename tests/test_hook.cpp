#include "io_context.h"
#include "util.h"

#include <arpa/inet.h>
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
        auto ret = read(sock, buf, sizeof(buf));
        if (ret <= 0) {
            spdlog::debug("{} disconnect", sock);
            break;
        }
        spdlog::debug("{} read {}: {}", sock, ret, buf);
        write(sock, response.c_str(), response.size());
    }
    close(sock);
}

void test_socket() {
    spdlog::info("test_socket");

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    checkRet(sock);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);

    checkRet(bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)));
    checkRet(listen(sock, SOMAXCONN));

    spdlog::info("Listening on port 8080...");
    while (true) {
        sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_sock = accept(sock, reinterpret_cast<sockaddr*>(&client_addr), &addr_len);
        checkRet(client_sock);
        IOContext::spawn(([client_sock]() { handle(client_sock); }));
    }
}

int main() {
    spdlog::set_level(spdlog::level::debug);
    sylar::IOContext scheduler(true);
    scheduler.schedule(test_socket);
    scheduler.execute();
}
