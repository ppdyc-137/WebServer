#include "socket.h"
#include "file_descriptor.h"
#include "hook.h"

#include <cstring>
#include <memory>
#include <spdlog/spdlog.h>
#include <sys/socket.h>

namespace sylar {
    std::shared_ptr<Socket> Socket::newSocket(int family, int type, int protocol) {
        return std::shared_ptr<Socket>{new Socket(family, type, protocol)};
    }

    std::shared_ptr<Socket> Socket::newTcpSocket() { return newSocket(AF_INET, SOCK_STREAM); }
    std::shared_ptr<Socket> Socket::newUdpSocket() { return newSocket(AF_INET, SOCK_DGRAM); }

    Socket::Socket(int family, int type, int protocol)
        : family_(family), type_(type), protocol_(protocol), fd_(socket(family, type, protocol)) {}

    Socket::Socket(int fd, int family, int type, int protocol)
        : family_(family), type_(type), protocol_(protocol), fd_(fd) {}

    void Socket::setSendTimeout(uint64_t timeout) const {
        auto file_descriptor = FDManager::getInstance().get(fd_);
        if (file_descriptor) {
            file_descriptor->send_timeout_ = timeout;
        }
    }
    uint64_t Socket::getSendTimeout() const {
        auto file_descriptor = FDManager::getInstance().get(fd_);
        if (file_descriptor) {
            return file_descriptor->send_timeout_;
        }
        return TIMEOUT_INFINITY;
    }
    void Socket::setRecvTimeout(uint64_t timeout) const {
        auto file_descriptor = FDManager::getInstance().get(fd_);
        if (file_descriptor) {
            file_descriptor->recv_timeout_ = timeout;
        }
    }
    uint64_t Socket::getRecvTimeout() const {
        auto file_descriptor = FDManager::getInstance().get(fd_);
        if (file_descriptor) {
            return file_descriptor->recv_timeout_;
        }
        return TIMEOUT_INFINITY;
    }

    bool Socket::checkAddress(const Address& addr) const {
        if (!valid()) {
            errno = EBADF;
            return false;
        }
        if (addr.getFamily() != family_) {
            errno = EAFNOSUPPORT;
            return false;
        }
        return true;
    }

    std::shared_ptr<Socket> Socket::accept() const {
        int sock = ::accept(fd_, nullptr, nullptr);
        if (sock == -1) {
            spdlog::error("Socket {} accept error: {}", fd_, strerror(errno));
        }
        return std::shared_ptr<Socket>{new Socket(sock, family_, type_, protocol_)};
    }

    bool Socket::bind(const Address& addr) {
        if (!checkAddress(addr)) {
            return false;
        }

        auto ret = ::bind(fd_, addr.getAddr(), addr.getAddrLen());
        if (ret < 0) {
            spdlog::error("Socket {} bind {} error: {}", fd_, addr.toString(), strerror(errno));
            return false;
        }

        return true;
    }

    bool Socket::connect(const Address& addr, uint64_t timeout) {
        if (!checkAddress(addr)) {
            return -1;
        }

        auto ret = ::connect(fd_, addr.getAddr(), addr.getAddrLen(), timeout);
        if (ret < 0) {
            spdlog::error("Socket {} connect {} error: {}", fd_, addr.toString(), strerror(errno));
            return false;
        }
        return true;
    }

    bool Socket::listen(int backlog) {
        if (!valid()) {
            errno = EBADF;
            return false;
        }

        auto ret = ::listen(fd_, backlog);
        if (ret < 0) {
            spdlog::error("Socket {} listen error: {}", fd_, strerror(errno));
            return false;
        }

        return true;
    }

    bool Socket::close() {
        if (!valid()) {
            errno = EBADF;
            return false;
        }

        auto ret = ::close(fd_);
        if (ret < 0) {
            spdlog::error("Socket {} close error: {}", fd_, strerror(errno));
            return false;
        }
        return true;
    }

    ssize_t Socket::send(const void* buf, size_t len, int flags) { return ::send(fd_, buf, len, flags); }
    ssize_t Socket::recv(void* buf, size_t len, int flags) { return ::recv(fd_, buf, len, flags); }

    std::shared_ptr<Address> Socket::getPeerAddress() const {
        sockaddr_storage addr{};
        socklen_t len{};
        switch (family_) {
        case AF_INET:
            len = sizeof(sockaddr_in);
            break;
        case AF_INET6:
            len = sizeof(sockaddr_in6);
            break;
        default:
            len = sizeof(sockaddr_storage);
            break;
        }
        auto ret = getpeername(fd_, reinterpret_cast<sockaddr*>(&addr), &len);
        if (ret < 0) {
            spdlog::error("Socket {} getpeername error: {}", fd_, strerror(errno));
            return nullptr;
        }
        return Address::newAddress(reinterpret_cast<sockaddr*>(&addr), len);
    }

} // namespace sylar
