#include <boost/asio.hpp>
#include <iostream>

using boost::asio::ip::tcp;

const std::string response = "HTTP/1.1 200 OK\r\n"
                             "Content-Type: text/html\r\n"
                             "Content-Length: 13\r\n"
                             "\r\n"
                             "Hello, world!";

void session(tcp::socket sock) {
    try {
        char data[1024];
        while (true) {
            boost::system::error_code error;
            size_t length = sock.read_some(boost::asio::buffer(data), error);
            if (error == boost::asio::error::eof) {
                break; // Connection closed cleanly by peer.
            } else if (error) {
                throw boost::system::system_error(error); // Some other error.
            }

            boost::asio::write(sock, boost::asio::buffer(response));
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception in thread: " << e.what() << "\n";
    }
}

void server(boost::asio::io_context& io_context, unsigned short port) {
    tcp::acceptor a(io_context, tcp::endpoint(tcp::v4(), port));
    while (true) {
        tcp::socket sock(io_context);
        a.accept(sock);
        std::thread(session, std::move(sock)).detach();
    }
}

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: asio_server <port>\n";
            return 1;
        }

        boost::asio::io_context io_context;
        server(io_context, std::atoi(argv[1]));
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
