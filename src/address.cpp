#include "address.h"
#include "util.h"

#include <arpa/inet.h>
#include <cstring>
#include <endian.h>
#include <memory>
#include <netdb.h>
#include <spdlog/spdlog.h>
#include <string>

namespace {
    std::shared_ptr<addrinfo> getAddrInfo(const std::string& host, const std::string& service, const addrinfo& hints) {
        addrinfo* res{};
        auto ret = getaddrinfo(host.c_str(), service.c_str(), &hints, &res);
        if (ret < 0) {
            spdlog::error("getaddrinfo error: host: {}, service: {}: {}", host, service, gai_strerror(ret));
            return nullptr;
        }
        if (res == nullptr) {
            spdlog::error("getaddrinfo return nullptr: host: {}, service: {}", host, service);
            return nullptr;
        }
        return {res, [](auto* x) { freeaddrinfo(x); }};
    }

    std::string ipAddrToString(const sockaddr* addr, socklen_t len) {
        char host[NI_MAXHOST];
        auto ret = getnameinfo(addr, len, host, sizeof(host), nullptr, 0, NI_NUMERICHOST);
        if (ret < 0) {
            spdlog::error("getnameinfo error: {}", gai_strerror(ret));
            return "";
        }
        return host;
    }
} // namespace

namespace sylar {
    std::shared_ptr<Address> Address::newAddress(const sockaddr* addr, socklen_t) {
        if (addr == nullptr) {
            return nullptr;
        }

        switch (addr->sa_family) {
        case AF_INET:
            return std::make_shared<IPV4Address>(*reinterpret_cast<const sockaddr_in*>(addr));
        case AF_INET6:
            return std::make_shared<IPV6Address>(*reinterpret_cast<const sockaddr_in6*>(addr));
        default:
            return nullptr;
            // return std::make_shared<UnknownAddress>(addr, len);
        }
    }

    IPV4Address::IPV4Address(const std::string& name, uint16_t port) {
        addrinfo hints{};
        hints.ai_flags = AI_NUMERICSERV;
        hints.ai_family = AF_INET;

        auto res = getAddrInfo(name, std::to_string(port), hints);
        if (res != nullptr) {
            std::memcpy(&addr_, res->ai_addr, sizeof(addr_));
        }
    }

    IPV4Address::IPV4Address(const std::string& host, const std::string& service) {
        addrinfo hints{};
        hints.ai_flags = AI_ALL;
        hints.ai_family = AF_INET;

        auto res = getAddrInfo(host, service, hints);
        if (res != nullptr) {
            std::memcpy(&addr_, res->ai_addr, sizeof(addr_));
        }
    }

    std::string IPV4Address::getIP() const { return ipAddrToString(getAddr(), sizeof(addr_)); }

    IPV6Address::IPV6Address(const std::string& name, uint16_t port) {
        addrinfo hints{};
        hints.ai_flags = AI_NUMERICSERV;
        hints.ai_family = AF_INET6;

        auto res = getAddrInfo(name, std::to_string(port), hints);
        if (res != nullptr) {
            std::memcpy(&addr_, res->ai_addr, sizeof(addr_));
        }
    }

    IPV6Address::IPV6Address(const std::string& host, const std::string& service) {
        addrinfo hints{};
        hints.ai_flags = AI_ALL;
        hints.ai_family = AF_INET6;

        auto res = getAddrInfo(host, service, hints);
        if (res != nullptr) {
            std::memcpy(&addr_, res->ai_addr, sizeof(addr_));
        }
    }

    std::string IPV6Address::getIP() const { return ipAddrToString(getAddr(), sizeof(addr_)); }

    UnknownAddress::UnknownAddress(const sockaddr* addr, socklen_t len) : len_(len) {
        SYLAR_ASSERT(len <= sizeof(addr_));
        std::memcpy(&addr_, addr, len);
    }

} // namespace sylar
