#include "util.h"

void foo() {
    SYLAR_ASSERT2(false, "foo");
}

int main() {
    foo();
}
