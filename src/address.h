#pragma once

#include <cstdint>
#include <memory>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

namespace sylar {
    class Address {
    public:
        static std::shared_ptr<Address> newAddress(const sockaddr* addr, socklen_t len);
        virtual ~Address() = default;

        virtual const sockaddr* getAddr() const = 0;
        virtual socklen_t getAddrLen() const = 0;
        virtual sa_family_t getFamily() const = 0;
    };

    class IPAddress : public Address {
    public:
        virtual std::string getIP() const = 0;
        virtual uint16_t getPort() const = 0;
    };

    class IPV4Address : public IPAddress {
    public:
        explicit IPV4Address(const sockaddr_in& addr) : addr_(addr) {}
        IPV4Address(const std::string& name, uint16_t port);
        IPV4Address(const std::string& host, const std::string& service);

        const sockaddr* getAddr() const override { return reinterpret_cast<const sockaddr*>(&addr_); }
        socklen_t getAddrLen() const override { return sizeof(addr_); }
        sa_family_t getFamily() const override { return addr_.sin_family; }

        std::string getIP() const override;
        uint16_t getPort() const override { return ntohs(addr_.sin_port); }

    private:
        sockaddr_in addr_{};
    };

    class IPV6Address : public IPAddress {
    public:
        explicit IPV6Address(const sockaddr_in6& addr) : addr_(addr) {}
        IPV6Address(const std::string& name, uint16_t port);
        IPV6Address(const std::string& host, const std::string& service);

        const sockaddr* getAddr() const override { return reinterpret_cast<const sockaddr*>(&addr_); }
        socklen_t getAddrLen() const override { return sizeof(addr_); }
        sa_family_t getFamily() const override { return addr_.sin6_family; }

        std::string getIP() const override;
        uint16_t getPort() const override { return ntohs(addr_.sin6_port); }

    private:
        sockaddr_in6 addr_{};
    };

    class UnknownAddress : public Address {
    public:
        UnknownAddress(const sockaddr* addr, socklen_t len);
        const sockaddr* getAddr() const override { return reinterpret_cast<const sockaddr*>(&addr_); }
        socklen_t getAddrLen() const override { return len_; }
        sa_family_t getFamily() const override { return addr_.ss_family; }

    private:
        sockaddr_storage addr_{};
        socklen_t len_{};
    };

} // namespace sylar
