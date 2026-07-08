#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <string_view>
#include <functional>
#include <tuple>
#include <utility>

import loki23;
import loki23.factory;

using Clock = std::chrono::high_resolution_clock;

static volatile long long g_sink = 0;

struct Result {
    const char* name;
    double      total_ns;
    long long   total_ops;
    long long   ops_per_sec;
};

constexpr int BENCH_RUNS = 1000;

static bool csv_enabled() {
    return std::getenv("BENCH_CSV") != nullptr;
}

static const char* suite_name() {
    const char* suite = std::getenv("BENCH_SUITE");
    return suite ? suite : "loki23";
}

static const char* csv_path() {
    return std::getenv("BENCH_CSV_OUT");
}

static void append_csv_result(const Result& r) {
    const char* path = csv_path();
    if (!path)
        return;

    if (FILE* file = std::fopen(path, "a")) {
        std::fprintf(file, "\"%s\",\"%s\",%d,%lld,%.0f,%.6f,%lld,\"\"\n",
                     suite_name(), r.name, BENCH_RUNS, r.total_ops, r.total_ns,
                     r.total_ns / 1e6, r.ops_per_sec);
        std::fclose(file);
    }
}

template <typename F>
static Result run(const char* name, long long N, F body) {
    body();

    double total_ns = 0.0;
    for (int run = 0; run < BENCH_RUNS; ++run) {
        auto t0 = Clock::now();
        for (long long i = 0; i < N; ++i)
            body();
        auto t1 = Clock::now();

        total_ns += static_cast<double>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    }

    long long total_ops = N * static_cast<long long>(BENCH_RUNS);
    long long ops_sec = static_cast<long long>(
        static_cast<double>(total_ops) * 1e9 / total_ns);
    return {name, total_ns, total_ops, ops_sec};
}

static void print(const Result& r) {
    if (csv_enabled()) {
        std::printf("\"%s\",\"%s\",%d,%lld,%.0f,%.6f,%lld,\"\"\n",
                    suite_name(), r.name, BENCH_RUNS, r.total_ops, r.total_ns,
                    r.total_ns / 1e6, r.ops_per_sec);
        return;
    }

    append_csv_result(r);
    std::printf("  %-42s  %10.3f ms total   %12lld ops/s\n",
                r.name, r.total_ns / 1e6, r.ops_per_sec);
}

struct Widget {
    int value;
    explicit Widget(int v = 0) : value(v) {}
    virtual ~Widget() = default;
};

using PtrRC = SmartPtr<int,    RefCounted, NoCheck>;
using PtrDC = SmartPtr<Widget, DeepCopy,   NoCheck>;
using PtrNC = SmartPtr<int,    NoCopy,     NoCheck>;
#ifndef _WIN32
using InvalidFdResource = SmartPtr<int, UniqueOwnership, NoCheck, FileDescriptorStorage>;
#endif

struct Config { int x = 7; };

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

struct MMBase { virtual ~MMBase() = default; };
struct MMA : MMBase { int value = 1; };
struct MMB : MMBase { int value = 2; };
struct MMC : MMBase { int value = 3; };

int main() {
    if (!csv_enabled())
        std::printf("=== loki23 (C++23 modules rewrite) ===\n\n");

    constexpr long long N_PTR  = 1'000'000;
    constexpr long long N_SING = 10'000'000;
    constexpr long long N_FACT = 1'000'000;
    constexpr long long N_SETUP = 1'000;

    print(run("SmartPtr<RefCounted> ctor+copy+deref+dtor", N_PTR, [&] {
        PtrRC p{new int{42}};
        PtrRC q{p};
        g_sink += *p + *q;
    }));

    // Use (int)g_sink as the initial value so the compiler cannot constant-fold.
    print(run("SmartPtr<DeepCopy> ctor+clone+deref+dtor", N_PTR, [&] {
        PtrDC p{new Widget{(int)g_sink}};
        PtrDC q{p};
        g_sink += (*p).value + (*q).value;
    }));

    print(run("SmartPtr<NoCopy> unique ctor+deref+dtor", N_PTR, [&] {
        PtrNC p{new int{42}};
        g_sink += *p;
    }));

    print(run("SmartPtr<NoCopy> unique ctor+move+dtor", N_PTR, [&] {
        PtrNC p{new int{42}};
        PtrNC q{std::move(p)};
        g_sink += *q;
    }));

    print(run("SmartPtr<RefCounted> ctor+move+dtor", N_PTR, [&] {
        PtrRC p{new int{42}};
        PtrRC q{std::move(p)};
        g_sink += *q;
    }));

    print(run("SmartPtr<RefCounted> 10x move chain", N_PTR / 10, [&] {
        PtrRC p0{new int{0}};
        PtrRC p1{std::move(p0)}, p2{std::move(p1)}, p3{std::move(p2)},
              p4{std::move(p3)}, p5{std::move(p4)}, p6{std::move(p5)},
              p7{std::move(p6)}, p8{std::move(p7)}, p9{std::move(p8)};
        g_sink += *p9;
    }));

#ifndef _WIN32
    print(run("Resource<FileDescriptorStorage> invalid handle", N_PTR, [&] {
        InvalidFdResource fd{-1};
        g_sink += fd.valid() ? 1 : 0;
    }));
#endif

    // NoDestroy avoids atexit cleanup racing with module teardown.
    print(run("Singleton hot loop", N_SING, [&] {
        g_sink += SingletonHolder<Config, CreateUsingNew<Config>, NoDestroy<Config>>::instance().x;
    }));

    {
        Factory<Shape, Shapes> factory;
        const char* names[] = {"Circle", "Square", "Triangle", "Pentagon"};
        int idx = 0;

        print(run("Factory create by name", N_FACT, [&] {
            auto s = factory.create(names[idx & 3]);
            g_sink += s->area();
            ++idx;
        }));

        print(run("Factory create by hot name", N_FACT, [&] {
            auto s = factory.create("Circle");
            g_sink += s->area();
        }));

        idx = 0;
        print(run("Factory contains", N_FACT, [&] {
            g_sink += factory.contains(names[idx & 3]) ? 1 : 0;
            ++idx;
        }));
    }

    print(run("Factory setup+create by name", N_SETUP, [&] {
        Factory<Shape, Shapes> factory;
        auto s = factory.create("Circle");
        g_sink += s->area();
    }));

    {
        MMA a;
        MMB b;
        MMC c;
        MMBase* pairs[][2] = {{&a, &b}, {&b, &a}, {&c, &c}, {&a, &b}};
        int idx = 0;

        using StaticMM = StaticDispatcher<DispatchPolicy::FIRST_MATCH, NoOpNoMatchPolicy,
                                          MMBase, MMBase, std::tuple<void>, MMA, MMB, MMC>;
        auto execs = std::make_tuple(
            [](MMA* lhs, MMB* rhs) { g_sink += lhs->value + rhs->value; },
            [](MMB* lhs, MMA* rhs) { g_sink += lhs->value - rhs->value; },
            [](MMC* lhs, MMC* rhs) { g_sink += lhs->value * rhs->value; });

        print(run("Multimethod StaticDispatcher", N_FACT, [&] {
            StaticMM::dispatch(pairs[idx & 3][0], pairs[idx & 3][1], execs);
            ++idx;
        }));

        DynamicDispatcher<DispatchPolicy::FIRST_MATCH, NoOpNoMatchPolicy, MMBase&, MMBase&> dispatcher;
        dispatcher.add(std::function<void(MMA&, MMB&)>{[](MMA& lhs, MMB& rhs) {
            g_sink += lhs.value + rhs.value;
        }});
        dispatcher.add(std::function<void(MMB&, MMA&)>{[](MMB& lhs, MMA& rhs) {
            g_sink += lhs.value - rhs.value;
        }});
        dispatcher.add(std::function<void(MMC&, MMC&)>{[](MMC& lhs, MMC& rhs) {
            g_sink += lhs.value * rhs.value;
        }});
        idx = 0;

        print(run("Multimethod DynamicDispatcher", N_FACT, [&] {
            dispatcher.dispatch(*pairs[idx & 3][0], *pairs[idx & 3][1]);
            ++idx;
        }));

        print(run("Multimethod dynamic same pair", N_FACT, [&] {
            dispatcher.dispatch(a, b);
        }));
    }

    if (!csv_enabled())
        std::printf("\n(g_sink=%lld)\n", g_sink);
}
