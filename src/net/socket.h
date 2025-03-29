#pragma once

#include "address.h"
#include "timer.h"

#include <memory>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

namespace sylar {
    class Socket : std::enable_shared_from_this<Socket> {
    public:
        static std::shared_ptr<Socket> newSocket(int family, int type, int protocol = 0);
        static std::shared_ptr<Socket> newTcpSocket();
        static std::shared_ptr<Socket> newUdpSocket();

        Socket(Socket&& that) noexcept
            : family_(that.family_), type_(that.type_), protocol_(that.protocol_), fd_(std::exchange(that.fd_, -1)) {}
        ~Socket() {
            if (fd_ != -1) {
                close();
            }
        }

        void setSendTimeout(uint64_t timeout) const;
        uint64_t getSendTimeout() const;
        void setRecvTimeout(uint64_t timeout) const;
        uint64_t getRecvTimeout() const;

        // int getOption(int level, int option, void* result, socklen_t* len);
        // int setOption(int level, int option, const void* result, socklen_t len);

        std::shared_ptr<Socket> accept() const;
        bool bind(const Address& addr);
        bool connect(const Address& addr, uint64_t timeout = TIMEOUT_INFINITY);
        bool listen(int backlog = SOMAXCONN);
        bool close();

        ssize_t send(const void* buf, size_t len, int flags = 0);
        ssize_t recv(void* buf, size_t len, int flags = 0);

        std::shared_ptr<Address> getSockAddress() const;
        std::shared_ptr<Address> getPeerAddress() const;

        bool valid() const { return fd_ != -1; }
        explicit operator bool() const { return valid(); }

        int family_{};
        int type_{};
        int protocol_{};

    private:
        Socket(int family, int type, int protocol);
        Socket(int fd, int family, int type, int protocol);

        bool checkAddress(const Address& addr) const;

        int fd_{-1};
    };

} // namespace sylar
