#include "iomanager.h"

#include <arpa/inet.h>
#include <sys/socket.h>

#include <spdlog/spdlog.h>

void foo() {
    for (int i = 0; i < 3; i++) {
        spdlog::info("hello {}", i);
        sleep(1);
    }
}

void test_socket() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "1.1.1.1", &addr.sin_addr.s_addr);

    {
        auto ret = connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        if (ret < 0) {
            spdlog::error("connect error: {}", std::strerror(errno));
            return;
        }
        spdlog::info("connect return: {}", ret);
    }

    {
        auto buf = "GET / HTTP/1.1\r\n\r\n";
        auto ret = write(sock, buf, strlen(buf) + 1);
        if (ret < 0) {
            spdlog::error("write error: {}", std::strerror(errno));
            return;
        }
        spdlog::info("write return: {}", ret);
    }

    {
        char buf[1024];
        auto ret = read(sock, buf, sizeof(buf));
        if (ret < 0) {
            spdlog::error("read error {}", std::strerror(errno));
            return;
        }
        spdlog::info("read {}:\n{}", ret, buf);
    }
    close(sock);
}

int main() {
    spdlog::set_level(spdlog::level::debug);

    sylar::IOManager scheduler(3, "Scheduler");
    scheduler.start();

    // scheduler.schedule(foo, 3);
    scheduler.schedule(test_socket);
}
