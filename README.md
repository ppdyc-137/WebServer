```cpp
#include "file/socket.h"
#include "io_context.h"

#include <spdlog/spdlog.h>

using namespace async;

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
        socket_write(sock, read);
    }
    file_close(std::move(sock));
}

void test_socket() {
    spdlog::info("test_socket");

    auto sockfd = socket_listen(*AddressResolver().host("127.0.0.1").port(8080).resolve_one(), SOMAXCONN).releaseFile();
    spdlog::info("Listening on port 8080...");
    while (true) {
        auto client_sock = socket_accept(sockfd).releaseFile();
        IOContext::spawn(([client_sock]() { handle(client_sock); }));
    }
}

int main() {
    async::IOContext scheduler(6);
    scheduler.spawn(test_socket);
    scheduler.execute();
}
```
