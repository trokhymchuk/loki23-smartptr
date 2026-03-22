module;

#include <unistd.h>

export module loki23.smart_ptr.resource;

export import loki23.smart_ptr; // Re-exports SmartPtr + all policies

// ============================================================
// POSIX storage policies
// T is a phantom type (handle is always int); required by the
// template<typename> typename StorePolicy parameter of SmartPtr.
// ============================================================

export template <typename T>
struct FileDescriptorStorage {
    using handle_type = int;
    handle_type handle_{-1};

    handle_type get()           const noexcept { return handle_; }
    void        set(handle_type h)    noexcept { handle_ = h; }

    static constexpr handle_type null_handle() noexcept { return -1; }
    static bool is_valid(handle_type h) noexcept { return h >= 0; }
    static void destroy(handle_type h)  noexcept { if (h >= 0) ::close(h); }
};

export template <typename T>
struct SocketStorage {
    using handle_type = int;
    handle_type handle_{-1};

    handle_type get()           const noexcept { return handle_; }
    void        set(handle_type h)    noexcept { handle_ = h; }

    static constexpr handle_type null_handle() noexcept { return -1; }
    static bool is_valid(handle_type h) noexcept { return h >= 0; }
    static void destroy(handle_type h)  noexcept { if (h >= 0) ::close(h); }
};
