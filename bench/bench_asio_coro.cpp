#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/write.hpp>
#include <iostream>

using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::ip::tcp;
namespace this_coro = boost::asio::this_coro;

const std::string response = "HTTP/1.1 200 OK\r\n"
                             "Content-Type: text/html\r\n"
                             "Content-Length: 13\r\n"
                             "\r\n"
                             "Hello, world!";

awaitable<void> session(tcp::socket socket) {
    try {
        char data[1024];
        for (;;) {
            std::size_t n = co_await socket.async_read_some(boost::asio::buffer(data), boost::asio::use_awaitable);
            co_await boost::asio::async_write(socket, boost::asio::buffer(response), boost::asio::use_awaitable);
        }
    } catch (const std::exception& e) {
        if (std::string(e.what()) != "End of file") {
            std::printf("session exception: %s\n", e.what());
        }
    }
}

awaitable<void> listener(short port) {
    auto executor = co_await this_coro::executor;
    tcp::acceptor acceptor(executor, {tcp::v4(), port});
    for (;;) {
        tcp::socket socket = co_await acceptor.async_accept(boost::asio::use_awaitable);
        co_spawn(executor, session(std::move(socket)), detached);
    }
}

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: asio_coro_server <port>\n";
            return 1;
        }

        boost::asio::io_context io_context(1);

        co_spawn(io_context, listener(std::atoi(argv[1])), detached);

        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&](auto, auto) { io_context.stop(); });

        io_context.run();
    } catch (const std::exception& e) {
        std::printf("Exception: %s\n", e.what());
    }

    return 0;
}
