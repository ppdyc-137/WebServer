#include "util.h"
#include <iostream>

void foo() {
    auto bt = sylar::backTrace(10);
    for (auto& s : bt) {
        std::cout << s << '\n';
    }
}

int main() {
    foo();
}
