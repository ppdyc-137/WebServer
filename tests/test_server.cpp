#include "io_context.h"
#include "file/socket.h"

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

void test_socket() {
    spdlog::info("test_socket");

    auto sock = socket_listen(*AddressResolver().host("127.0.0.1").port(8080).resolve_one(), SOMAXCONN);
    spdlog::info("Listening on port 8080...");
    while (true) {
        auto client_sock = socket_accept(sock).releaseFile();
        IOContext::spawn(([client_sock]() { handle(client_sock); }));
    }
}

int main() {
    // spdlog::set_level(spdlog::level::debug);
    sylar::IOContext scheduler;
    scheduler.schedule(test_socket);
    scheduler.execute();
}
