#include "socket.h"
#include "util.h"

#include <netdb.h>
#include <optional>
#include <spdlog/spdlog.h>
#include <sys/socket.h>

namespace sylar {
    std::optional<AddressResolver::ResolveResult> AddressResolver::resolve_all() {
        if (host_.empty()) {
            return std::nullopt;
        }

        const char* service{};
        if (!service_.empty()) {
            service = service_.c_str();
        } else if (!port_.empty()) {
            service = port_.c_str();
        }

        struct addrinfo* result = nullptr;
        int ret = getaddrinfo(host_.c_str(), service, &hints_, &result);
        if (ret) {
            spdlog::error("getaddrinfo: {}", gai_strerror(ret));
            return std::nullopt;
        }

        ResolveResult res;
        for (struct addrinfo* rp = result; rp != nullptr; rp = rp->ai_next) {
            res.addrs_.emplace_back(rp->ai_addr, rp->ai_addrlen, rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        }

        freeaddrinfo(result);
        return res;
    }

    std::optional<SocketAddress> AddressResolver::resolve_one() {
        auto res = resolve_all();
        if (!res.has_value() || res->addrs_.empty()) {
            return std::nullopt;
        }
        return res->addrs_.front();
    }

    SocketAddress::SocketAddress(struct sockaddr const* addr, socklen_t addrLen, sa_family_t family, int sockType,
                                 int protocol)
        : type_(sockType), protocol_(protocol), len_(addrLen) {
        std::memcpy(&addr_, addr, addrLen);
        addr_.ss_family = family;
    }

    std::string SocketAddress::host() const {
        if (family() == AF_INET) {
            const auto& sin = reinterpret_cast<struct sockaddr_in const&>(addr_).sin_addr;
            char buf[INET_ADDRSTRLEN] = {};
            inet_ntop(family(), &sin, buf, sizeof(buf));
            return buf;
        }
        if (family() == AF_INET6) {
            const auto& sin6 = reinterpret_cast<struct sockaddr_in6 const&>(addr_).sin6_addr;
            char buf[INET6_ADDRSTRLEN] = {};
            inet_ntop(AF_INET6, &sin6, buf, sizeof(buf));
            return buf;
        }

        throw std::runtime_error("address family not ipv4 or ipv6");
    }

    int SocketAddress::port() const {
        if (family() == AF_INET) {
            auto port = reinterpret_cast<struct sockaddr_in const&>(addr_).sin_port;
            return ntohs(port);
        }
        if (family() == AF_INET6) {
            auto port = reinterpret_cast<struct sockaddr_in6 const&>(addr_).sin6_port;
            return ntohs(port);
        }

        throw std::runtime_error("address family not ipv4 or ipv6");
    }

    std::string SocketAddress::toString() const { return host() + ':' + std::to_string(port()); }

    SocketAddress getSockAddr(SocketHandle& sock) {
        SocketAddress addr;
        checkRet(getsockname(sock.fileNo(), addr.raw_addr(), &addr.len_));
        return addr;
    }
    SocketAddress getPeerAddr(SocketHandle& sock) {
        SocketAddress addr;
        checkRet(getpeername(sock.fileNo(), addr.raw_addr(), &addr.len_));
        return addr;
    }

    SocketHandle createSocket(int family, int type, int protocol) {
        int fd = UringOp().prep_socket(family, type, protocol).await();
        return SocketHandle{fd};
    }

    SocketHandle socket_bind(SocketAddress const& addr) {
        SocketHandle sock = createSocket(addr.family(), addr.socktype(), addr.protocol());
        checkRet(bind(sock.fileNo(), addr.raw_addr(), addr.len_));
        return sock;
    }
    SocketListener socket_listen(SocketAddress const& addr, int backlog) {
        auto sock = socket_bind(addr);
        return socket_listen(std::move(sock), backlog);
    }
    SocketListener socket_listen(SocketHandle sock, int backlog) {
        SocketListener serv(sock.releaseFile());
        checkRet(listen(serv.fileNo(), backlog));
        return serv;
    }
    SocketHandle socket_accept(SocketListener& listener) {
        int fd = UringOp().prep_accept(listener.fileNo(), nullptr, nullptr, 0).await();
        return SocketHandle{fd};
    }

    SocketHandle socket_connect(SocketAddress const& addr) {
        SocketHandle sock = createSocket(addr.family(), addr.socktype(), addr.protocol());
        checkRet(UringOp().prep_connect(sock.fileNo(), addr.raw_addr(), addr.len_).await());
        return sock;
    }

    int socket_read(SocketHandle& sock, std::span<char> buffer) {
        return UringOp().prep_read(sock.fileNo(), buffer.data(), static_cast<unsigned int>(buffer.size()), 0).await();
    }
    int socket_write(SocketHandle& sock, std::span<char const> buffer) {
        return UringOp().prep_write(sock.fileNo(), buffer.data(), static_cast<unsigned int>(buffer.size()), 0).await();
    }

} // namespace sylar
