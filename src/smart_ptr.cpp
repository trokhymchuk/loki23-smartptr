#include <iostream>

import loki23;

int main() {
    using SmartPtr_t = SmartPtr<int, RefCounted>;
    SmartPtr_t sptr{new int{5}};
    SmartPtr_t sptr_copy{sptr};

    std::cout << sptr.get() << '\n';

    sptr = SmartPtr_t{};

    std::cout << sptr_copy.get() << '\n';

    // In-place construction: no explicit new needed
    SmartPtr_t sptr2(5);
    std::cout << *sptr2 << '\n';
}
