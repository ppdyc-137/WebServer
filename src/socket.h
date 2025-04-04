#pragma once

#include "file.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <string>

namespace sylar {
    struct SocketAddress {
        SocketAddress() = default;

        explicit SocketAddress(struct sockaddr const* addr, socklen_t addrLen, sa_family_t family, int sockType,
                               int protocol);

        sa_family_t family() const noexcept { return addr_.ss_family; }
        int socktype() const noexcept { return type_; }
        int protocol() const noexcept { return protocol_; }
        std::string host() const;
        int port() const;

        std::string toString() const;

        struct sockaddr* raw_addr() { return reinterpret_cast<struct sockaddr*>(&addr_); }
        struct sockaddr const* raw_addr() const { return reinterpret_cast<struct sockaddr const*>(&addr_); }

        int type_{};
        int protocol_{};
        socklen_t len_{sizeof(sockaddr_storage)};
        struct sockaddr_storage addr_{};
    };

    struct AddressResolver {
        AddressResolver& host(std::string_view host) {
            host_ = host;
            return *this;
        }

        AddressResolver& port(int port) {
            port_ = std::to_string(port);
            return *this;
        }

        AddressResolver& service(std::string_view service) {
            service_ = service;
            return *this;
        }

        AddressResolver& family(int family) {
            hints_.ai_family = family;
            return *this;
        }

        AddressResolver& socktype(int socktype) {
            hints_.ai_socktype = socktype;
            return *this;
        }

        struct ResolveResult {
            std::vector<SocketAddress> addrs_;
        };

        std::optional<ResolveResult> resolve_all();
        std::optional<SocketAddress> resolve_one();

    private:
        std::string host_;
        std::string port_;
        std::string service_;
        struct addrinfo hints_ = {};
    };

    struct [[nodiscard]] SocketHandle : FileHandle {
        using FileHandle::FileHandle;
    };

    struct [[nodiscard]] SocketListener : SocketHandle {
        using SocketHandle::SocketHandle;
    };

    SocketAddress getSockAddr(SocketHandle& sock);
    SocketAddress getPeerAddr(SocketHandle& sock);

    SocketHandle createSocket(int family, int type, int protocol);

    SocketHandle socket_bind(SocketAddress const& addr);
    SocketListener socket_listen(SocketAddress const& addr, int backlog);
    SocketListener socket_listen(SocketHandle sock, int backlog);

    SocketHandle socket_accept(SocketListener& listenr);

    SocketHandle socket_connect(SocketAddress const& addr);

    int socket_read(SocketHandle& sock, std::span<char> buf);
    int socket_write(SocketHandle& sock, std::span<char const> buf);

} // namespace sylar
