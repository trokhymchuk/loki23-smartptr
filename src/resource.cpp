#include <fcntl.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>

import loki23.resource;

// ============================================================
// Intrusive reference-counted base
// ============================================================
struct IntrusiveBase {
    void add_ref()  noexcept { ++m_refs; }
    int  release()  noexcept { return --m_refs; }
    int  ref_count() const noexcept { return m_refs; }
  private:
    int m_refs{0};
};

struct SensorData : IntrusiveBase {
    double temperature{};
    double pressure{};
    SensorData(double t, double p) : temperature(t), pressure(p) {}
};

// ============================================================
// Type aliases — behaviour defined entirely by policy selection.
// The Resource class template is never modified.
// ============================================================

// Fast path: unique ownership, no runtime checks, heap pointer
using FastResource    = Resource<int,        UniqueOwnership,   NoCheck,        PointerStorage>;

// Safe path: shared ownership, exception on invalid access, heap pointer
using SafeResource    = Resource<int,        SharedOwnership,   ExceptionCheck, PointerStorage>;

// POSIX file: unique ownership, assert on bad fd, OS file descriptor
using FileResource    = Resource<int,        UniqueOwnership,   AssertCheck,    FileDescriptorStorage>;

// Network socket: shared handle, exception on bad socket
using SocketResource  = Resource<int,        SharedOwnership,   ExceptionCheck, SocketStorage>;

// Intrusive: the managed object itself tracks reference count
using IntrusiveResource = Resource<SensorData, IntrusiveOwnership, AssertCheck, PointerStorage>;

// ============================================================
int main() {
    std::cout << "=== Policy-based Resource Manager ===\n\n";

    // ---------------------------------------------------------
    // [1] UniqueOwnership + PointerStorage + NoCheck
    // ---------------------------------------------------------
    std::cout << "[1] FastResource (Unique | NoCheck | Pointer)\n";
    {
        FastResource r{new int{42}};
        std::cout << "    value = " << *r << "\n";

        FastResource r2 = std::move(r);          // ownership transferred, r is now empty
        std::cout << "    after move: r.valid()="  << r.valid()
                  <<          "  r2.valid()=" << r2.valid() << "\n";
    }  // r2 destructor deletes the int

    // ---------------------------------------------------------
    // [2] SharedOwnership + PointerStorage + ExceptionCheck
    // ---------------------------------------------------------
    std::cout << "\n[2] SafeResource (Shared | ExceptionCheck | Pointer)\n";
    {
        SafeResource r1{new int{100}};
        SafeResource r2 = r1;
        SafeResource r3 = r2;
        std::cout << "    use_count = " << r1.use_count() << "\n";  // 3
        {
            SafeResource r4 = r3;
            std::cout << "    use_count (inner scope) = " << r1.use_count() << "\n";  // 4
        }
        std::cout << "    use_count (after inner scope) = " << r1.use_count() << "\n";  // 3

        try {
            SafeResource bad{};
            bad.get();  // throws: handle is invalid
        } catch (const std::runtime_error &e) {
            std::cout << "    caught: " << e.what() << "\n";
        }
    }

    // ---------------------------------------------------------
    // [3] UniqueOwnership + FileDescriptorStorage + AssertCheck
    // ---------------------------------------------------------
    std::cout << "\n[3] FileResource (Unique | AssertCheck | FileDescriptor)\n";
    {
        int fd = ::open("/tmp/loki23_demo.txt",
                        O_CREAT | O_WRONLY | O_TRUNC, 0644);
        FileResource file{fd};
        std::cout << "    fd=" << file.get() << "  valid=" << file.valid() << "\n";
    }  // file descriptor closed here automatically
    std::cout << "    fd closed on scope exit\n";

    // ---------------------------------------------------------
    // [4] SharedOwnership + SocketStorage + ExceptionCheck
    // ---------------------------------------------------------
    std::cout << "\n[4] SocketResource (Shared | ExceptionCheck | Socket)\n";
    {
        int sock = ::socket(AF_INET, SOCK_STREAM, 0);
        SocketResource s1{sock};
        SocketResource s2 = s1;  // both share the same socket fd
        std::cout << "    socket fd=" << s1.get()
                  << "  use_count=" << s1.use_count() << "\n";
    }  // socket closed when last owner leaves scope
    std::cout << "    socket closed by last owner\n";

    // ---------------------------------------------------------
    // [5] IntrusiveOwnership + PointerStorage + AssertCheck
    // ---------------------------------------------------------
    std::cout << "\n[5] IntrusiveResource (Intrusive | AssertCheck | Pointer)\n";
    {
        IntrusiveResource ir1{new SensorData{36.6, 101.3}};
        IntrusiveResource ir2 = ir1;
        IntrusiveResource ir3 = ir2;
        std::cout << "    temperature  = " << ir1->temperature << "\n";
        std::cout << "    ref_count    = " << ir1.use_count()  << "\n";  // 3
    }  // object destroyed when last IntrusiveResource leaves scope

    std::cout << "\n=== Done ===\n";
}
