#include "net/address.h"

#include <format>
#include <iostream>

int main() {
    auto addr = sylar::IPV4Address("mirrors.hit.edu.cn", 80);
    std::cout << std::format("{}:{}", addr.getIP(), addr.getPort()) << '\n';
}
