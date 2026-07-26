#include <cstdint>
#include <iostream>

import loki23;

// A large struct that won't fit in the default small buffer (32 bytes)
struct LargePayload {
    double data[8]; // 64 bytes
    explicit LargePayload(double v) { data[0] = v; }
    double value() const { return data[0]; }
};

// Alias binding a custom 4-byte buffer size
template <typename U> using TinyBuffer = SmallBufferStorage<U, 4>;

int main() {
    std::cout << "=== SmallBufferStorage Demo ===\n\n";

    // 1) int fits inline (4 bytes <= 32 bytes)
    {
        using Ptr = SmartPtr<int, UniqueOwnership, NoCheck, SmallBuffer>;
        static_assert(SmallBuffer<int>::fits_inline);

        Ptr p(42);
        std::cout << "[1] int (inline):  *p = " << *p << '\n'
                  << "    &p = " << &p << "  p.get() = " << p.get() << '\n';

        Ptr p2(std::move(p));
        std::cout << "    after move:    *p2 = " << *p2
                  << "  p.valid() = " << p.valid() << '\n'
                  << "    &p2 = " << &p2 << "  p2.get() = " << p2.get() << '\n';
    }

    // 2) double fits inline (8 bytes <= 32 bytes)
    {
        using Ptr = SmartPtr<double, UniqueOwnership, NoCheck, SmallBuffer>;
        static_assert(SmallBuffer<double>::fits_inline);

        Ptr p(3.14);
        std::cout << "\n[2] double (inline): *p = " << *p << '\n'
                  << "    &p = " << &p << "  p.get() = " << p.get() << '\n';
    }

    // 3) LargePayload spills to heap (64 bytes > 32 bytes)
    {
        using Ptr = SmartPtr<LargePayload, UniqueOwnership, NoCheck, SmallBuffer>;
        static_assert(!SmallBuffer<LargePayload>::fits_inline);

        Ptr p(2.718);
        std::cout << "\n[3] LargePayload (heap): p->value() = " << p->value() << '\n'
                  << "    &p = " << &p << "  p.get() = " << static_cast<void *>(p.get()) << '\n';

        Ptr p2(std::move(p));
        std::cout << "    after move:          p2->value() = " << p2->value()
                  << "  p.valid() = " << p.valid() << '\n'
                  << "    &p2 = " << &p2 << "  p2.get() = " << static_cast<void *>(p2.get()) << '\n';
    }

    // 4) int with a tiny 4-byte buffer — still fits inline
    {
        using Ptr = SmartPtr<int, UniqueOwnership, NoCheck, TinyBuffer>;
        static_assert(TinyBuffer<int>::fits_inline);

        Ptr p(99);
        std::cout << "\n[4] int (4-byte buffer, inline): *p = " << *p << '\n'
                  << "    &p = " << &p << "  p.get() = " << p.get() << '\n';
    }

    // 5) int64_t with a tiny 4-byte buffer — goes to heap
    {
        using Ptr = SmartPtr<int64_t, UniqueOwnership, NoCheck, TinyBuffer>;
        static_assert(!TinyBuffer<int64_t>::fits_inline);

        Ptr p(int64_t{123456789});
        std::cout << "\n[5] int64_t (4-byte buffer, heap): *p = " << *p << '\n'
                  << "    &p = " << &p << "  p.get() = " << p.get() << '\n';
    }

    std::cout << "\n=== Done ===\n";
}
