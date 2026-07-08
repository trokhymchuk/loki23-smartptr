#include <fcntl.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>

import loki23.smart_ptr;

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

using FastResource    = SmartPtr<int,        UniqueOwnership,   NoCheck,        DefaultStorage>;
using SafeResource    = SmartPtr<int,        SharedOwnership,   ExceptionCheck, DefaultStorage>;
using FileResource    = SmartPtr<int,        UniqueOwnership,   AssertCheck,    FileDescriptorStorage>;
using SocketResource  = SmartPtr<int,        SharedOwnership,   ExceptionCheck, SocketStorage>;
using IntrusiveResource = SmartPtr<SensorData, IntrusiveOwnership, AssertCheck, DefaultStorage>;

int main() {
    std::cout << "=== Policy-based Resource Manager ===\n\n";

    std::cout << "[1] FastResource (Unique | NoCheck | Pointer)\n";
    {
        FastResource r{new int{42}};
        std::cout << "    value = " << *r << "\n";

        FastResource r2 = std::move(r);
        std::cout << "    after move: r.valid()="  << r.valid()
                  <<          "  r2.valid()=" << r2.valid() << "\n";
    }

    std::cout << "\n[2] SafeResource (Shared | ExceptionCheck | Pointer)\n";
    {
        SafeResource r1{new int{100}};
        SafeResource r2 = r1;
        SafeResource r3 = r2;
        std::cout << "    use_count = " << r1.use_count() << "\n";
        {
            SafeResource r4 = r3;
            std::cout << "    use_count (inner scope) = " << r1.use_count() << "\n";
        }
        std::cout << "    use_count (after inner scope) = " << r1.use_count() << "\n";

        try {
            SafeResource bad{};
            bad.get();
        } catch (const std::runtime_error &e) {
            std::cout << "    caught: " << e.what() << "\n";
        }
    }

    std::cout << "\n[3] FileResource (Unique | AssertCheck | FileDescriptor)\n";
    {
        int fd = ::open("/tmp/loki23_demo.txt",
                        O_CREAT | O_WRONLY | O_TRUNC, 0644);
        FileResource file{fd};
        std::cout << "    fd=" << file.get() << "  valid=" << file.valid() << "\n";
    }
    std::cout << "    fd closed on scope exit\n";

    std::cout << "\n[4] SocketResource (Shared | ExceptionCheck | Socket)\n";
    {
        int sock = ::socket(AF_INET, SOCK_STREAM, 0);
        SocketResource s1{sock};
        SocketResource s2 = s1;
        std::cout << "    socket fd=" << s1.get()
                  << "  use_count=" << s1.use_count() << "\n";
    }
    std::cout << "    socket closed by last owner\n";

    std::cout << "\n[5] IntrusiveResource (Intrusive | AssertCheck | Pointer)\n";
    {
        IntrusiveResource ir1{new SensorData{36.6, 101.3}};
        IntrusiveResource ir2 = ir1;
        IntrusiveResource ir3 = ir2;
        std::cout << "    temperature  = " << ir1->temperature << "\n";
        std::cout << "    ref_count    = " << ir1.use_count()  << "\n";
    }

    std::cout << "\n=== Done ===\n";
}
