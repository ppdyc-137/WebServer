#include "io_context.h"
#include "stream/socket_stream.h"
#include "stream/stdio_stream.h"
#include "stream/stream.h"

#include <spdlog/spdlog.h>

using namespace sylar;

void test_stream() {
    while (true) {
        std::string line = stdio().getline('\n');
        stdio().putline(line);
    }
}

void handle(int fd) {
    auto stream = make_stream<SocketStream>(SocketHandle(fd));
    try {
        while (true) {
            auto line = stream.getline('\n');
            spdlog::info("{} read: {}", fd, line);
            stream.putline(line);
        }
    } catch (std::system_error& e) {
        if (e.code() == Stream::eof()) {
            spdlog::debug("{} disconnect", fd);
        }
    }
}

void test_socket_stream() {
    spdlog::info("test_socket");

    auto sock = socket_listen(*AddressResolver().host("127.0.0.1").port(8080).resolve_one(), SOMAXCONN);
    spdlog::info("Listening on port 8080...");
    while (true) {
        auto client_sock = socket_accept(sock).releaseFile();
        IOContext::spawn(([client_sock]() { handle(client_sock); }));
    }
}

int main() {
    spdlog::set_level(spdlog::level::debug);

    IOContext context;
    context.schedule(test_socket_stream);
    context.execute();
}
