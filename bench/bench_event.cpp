#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>

#include <arpa/inet.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <print>

const std::string response = "HTTP/1.1 200 OK\r\n"
                             "Content-Type: text/html\r\n"
                             "Content-Length: 13\r\n"
                             "\r\n"
                             "Hello, world!";

static void echo_read_cb(struct bufferevent* bev, void* ctx) {
    /* This callback is invoked when there is data to read on bev. */
    struct evbuffer* input = bufferevent_get_input(bev);
    evbuffer_drain(input, evbuffer_get_length(input));
    struct evbuffer* output = bufferevent_get_output(bev);

    evbuffer_add(output, response.c_str(), response.length());
}

static void echo_event_cb(struct bufferevent* bev, short events, void* ctx) {
    if (events & BEV_EVENT_ERROR) {
        perror("Error from bufferevent");
    }
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        bufferevent_free(bev);
    }
}

static void accept_conn_cb(struct evconnlistener* listener, evutil_socket_t fd, struct sockaddr* address, int socklen,
                           void* ctx) {
    /* We got a new connection! Set up a bufferevent for it. */
    struct event_base* base = evconnlistener_get_base(listener);
    struct bufferevent* bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

    bufferevent_setcb(bev, echo_read_cb, nullptr, echo_event_cb, nullptr);

    bufferevent_enable(bev, EV_READ | EV_WRITE);
}

static void accept_error_cb(struct evconnlistener* listener, void* ctx) {
    struct event_base* base = evconnlistener_get_base(listener);
    int err = EVUTIL_SOCKET_ERROR();
    std::cerr << "Accept error: " << evutil_socket_error_to_string(err) << "\n";
    event_base_loopexit(base, nullptr);
}

int main(int argc, char** argv) {
    struct event_base* base = nullptr;
    struct evconnlistener* listener = nullptr;
    struct sockaddr_in sin{};

    if (argc != 2) {
        std::cerr << "Usage: libevent_server <port>\n";
        return 1;
    }
    int port = atoi(argv[1]);

    base = event_base_new();
    if (!base) {
        std::cerr << "Couldn't create an event base\n";
        return 1;
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(0);
    sin.sin_port = htons(port);

    listener = evconnlistener_new_bind(base, accept_conn_cb, nullptr, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1,
                                       reinterpret_cast<struct sockaddr*>(&sin), sizeof(sin));
    if (!listener) {
        perror("Couldn't create listener");
        return 1;
    }
    evconnlistener_set_error_cb(listener, accept_error_cb);

    event_base_dispatch(base);
    return 0;
}
