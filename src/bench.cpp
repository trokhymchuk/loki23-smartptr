// Benchmark: loki23 (modern C++23 rewrite)

#include <chrono>
#include <cstdio>
#include <string_view>

import loki23;
import loki23.factory;

// ── helpers ──────────────────────────────────────────────────────────────────

using Clock = std::chrono::high_resolution_clock;

static volatile long long g_sink = 0;

struct Result {
    const char* name;
    double      ns_per_op;
    long long   ops_per_sec;
};

template <typename F>
static Result run(const char* name, long long N, F body) {
    body(); // warmup

    auto t0 = Clock::now();
    for (long long i = 0; i < N; ++i)
        body();
    auto t1 = Clock::now();

    double total_ns = static_cast<double>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    double ns_per_op  = total_ns / static_cast<double>(N);
    long long ops_sec = static_cast<long long>(1e9 / ns_per_op);
    return {name, ns_per_op, ops_sec};
}

static void print(const Result& r) {
    std::printf("  %-42s  %8.2f ns/op   %12lld ops/s\n",
                r.name, r.ns_per_op, r.ops_per_sec);
}

// ── SmartPtr fixtures ─────────────────────────────────────────────────────────

// loki23 DeepCopy uses the copy constructor — no Clone() method required.
// virtual dtor forces the compiler to actually heap-allocate (prevents elision).
struct Widget {
    int value;
    explicit Widget(int v = 0) : value(v) {}
    virtual ~Widget() = default;
};

using PtrRC = SmartPtr<int,    RefCounted, NoCheck>;
using PtrDC = SmartPtr<Widget, DeepCopy,   NoCheck>;

// ── Singleton fixture ─────────────────────────────────────────────────────────

struct Config { int x = 7; };

// ── Factory fixture ───────────────────────────────────────────────────────────

struct Shape { virtual int area() const = 0; virtual ~Shape() = default; };

struct Circle : Shape {
    static constexpr std::string_view type_name = "Circle";
    int area() const override { return 3; }
};
struct Square : Shape {
    static constexpr std::string_view type_name = "Square";
    int area() const override { return 4; }
};
struct Triangle : Shape {
    static constexpr std::string_view type_name = "Triangle";
    int area() const override { return 2; }
};
struct Pentagon : Shape {
    static constexpr std::string_view type_name = "Pentagon";
    int area() const override { return 5; }
};

using Shapes = type_list<Circle, Square, Triangle, Pentagon>;

// ── main ──────────────────────────────────────────────────────────────────────

int main() {
    std::printf("=== loki23 (C++23 modules rewrite) ===\n\n");

    constexpr long long N_PTR  = 1'000'000;
    constexpr long long N_SING = 10'000'000;
    constexpr long long N_FACT = 1'000'000;

    // --- SmartPtr RefCounted: construct + copy + dereference ---
    print(run("SmartPtr<RefCounted> ctor+copy+deref+dtor", N_PTR, [&] {
        PtrRC p{new int{42}};
        PtrRC q{p};
        g_sink += *p + *q;
    }));

    // --- SmartPtr DeepCopy: construct + copy-construct + dereference ---
    // Use (int)g_sink as the initial value so the compiler cannot constant-fold.
    print(run("SmartPtr<DeepCopy> ctor+clone+deref+dtor", N_PTR, [&] {
        PtrDC p{new Widget{(int)g_sink}};
        PtrDC q{p};               // heap-allocates a new Widget via copy ctor
        g_sink += (*p).value + (*q).value;
    }));

    // --- SmartPtr RefCounted: move (loki23 advantage) ---
    print(run("SmartPtr<RefCounted> ctor+move+dtor", N_PTR, [&] {
        PtrRC p{new int{42}};
        PtrRC q{std::move(p)};    // zero-cost: just pointer swap, no heap op
        g_sink += *q;
    }));

    // --- SmartPtr RefCounted: 10-deep copy chain ---
    print(run("SmartPtr<RefCounted> 10x copy chain", N_PTR / 10, [&] {
        PtrRC p0{new int{0}};
        PtrRC p1{p0}, p2{p1}, p3{p2}, p4{p3}, p5{p4},
              p6{p5}, p7{p6}, p8{p7}, p9{p8};
        g_sink += *p9;
    }));

    // --- Singleton hot access ---
    // Use NoDestroy to skip the atexit cleanup that can race with module teardown.
    print(run("Singleton::instance() hot loop", N_SING, [&] {
        g_sink += SingletonHolder<Config, CreateUsingNew<Config>, NoDestroy<Config>>::instance().x;
    }));

    // --- Factory: create by string_view key, use result ---
    {
        Factory<Shape, Shapes> factory;
        const char* names[] = {"Circle", "Square", "Triangle", "Pentagon"};
        int idx = 0;

        print(run("Factory::create(string_view)", N_FACT, [&] {
            auto s = factory.create(names[idx & 3]);
            g_sink += s->area();
            ++idx;
        }));
    }

    std::printf("\n(g_sink=%lld)\n", g_sink);
}
