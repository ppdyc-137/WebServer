#include "io_context.h"
#include "uring_op.h"
#include "util.h"
#include "stream/socket_stream.h"

#include <chrono>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>

using namespace sylar;

void handle(int fd) {
    auto stream = make_stream<SocketStream>(SocketHandle(fd));
    stream.timeout(std::chrono::seconds(10));
    try {
        while (true) {
            auto line = stream.getline('\n');
            spdlog::info("{} read: {}", fd, line);
            stream.putline(line);
        }
    } catch (Stream::EOFException& e) {
        spdlog::debug("{} disconnect", fd);
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

void test() {
    char buf[32];
    auto timeout = std::chrono::seconds(3);
    int ret = UringOp(timeout).prep_read(0, buf, sizeof(buf)).await();
    checkRetUring(ret);
}

int main() {
    spdlog::set_level(spdlog::level::debug);
    IOContext context;
    context.schedule(test_socket_stream);
    context.execute();
}
