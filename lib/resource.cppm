module;

#include <atomic>
#include <cassert>
#include <concepts>
#include <stdexcept>
#include <unistd.h>

export module loki23.resource;

// ============================================================
// Ownership Policy Tags
// ============================================================

export struct UniqueOwnership {};
export struct SharedOwnership {};
export struct IntrusiveOwnership {};

// ============================================================
// Checking Policies
// ============================================================

export struct NoCheck {
    static void check(bool) noexcept {}
};

export struct AssertCheck {
    static void check(bool valid) noexcept {
        assert(valid && "Resource: invalid handle");
    }
};

export struct ExceptionCheck {
    static void check(bool valid) {
        if (!valid)
            throw std::runtime_error("Resource: invalid handle");
    }
};

// ============================================================
// Storage Policies
// ============================================================

export template <typename T>
struct PointerStorage {
    using handle_type = T *;
    static constexpr handle_type null_handle() noexcept { return nullptr; }
    static bool is_valid(handle_type h) noexcept { return h != nullptr; }
    static void destroy(handle_type h) noexcept { delete h; }
};

export template <typename T>
struct FileDescriptorStorage {
    using handle_type = int;
    static constexpr handle_type null_handle() noexcept { return -1; }
    static bool is_valid(handle_type h) noexcept { return h >= 0; }
    static void destroy(handle_type h) noexcept {
        if (h >= 0)
            ::close(h);
    }
};

export template <typename T>
struct SocketStorage {
    using handle_type = int;
    static constexpr handle_type null_handle() noexcept { return -1; }
    static bool is_valid(handle_type h) noexcept { return h >= 0; }
    static void destroy(handle_type h) noexcept {
        if (h >= 0)
            ::close(h);
    }
};

// ============================================================
// Resource — forward declaration
// ============================================================

export template <typename T, typename OwnershipPolicy, typename CheckingPolicy,
                 template <typename> typename StoragePolicyT>
class Resource;

// ============================================================
// UniqueOwnership specialization — move-only
// ============================================================

export template <typename T, typename CheckingPolicy,
                 template <typename> typename StoragePolicyT>
class Resource<T, UniqueOwnership, CheckingPolicy, StoragePolicyT> {
    using Store     = StoragePolicyT<T>;
    using handle_type = typename Store::handle_type;

    handle_type m_handle;

  public:
    explicit Resource(handle_type h = Store::null_handle()) : m_handle(h) {
        CheckingPolicy::check(Store::is_valid(m_handle));
    }

    ~Resource() {
        if (Store::is_valid(m_handle))
            Store::destroy(m_handle);
    }

    Resource(const Resource &)            = delete;
    Resource &operator=(const Resource &) = delete;

    Resource(Resource &&other) noexcept : m_handle(other.m_handle) {
        other.m_handle = Store::null_handle();
    }
    Resource &operator=(Resource &&other) noexcept {
        if (this != &other) {
            if (Store::is_valid(m_handle))
                Store::destroy(m_handle);
            m_handle       = other.m_handle;
            other.m_handle = Store::null_handle();
        }
        return *this;
    }

    handle_type get() const {
        CheckingPolicy::check(Store::is_valid(m_handle));
        return m_handle;
    }
    bool valid() const noexcept { return Store::is_valid(m_handle); }

    T &operator*() requires std::same_as<handle_type, T *> {
        CheckingPolicy::check(Store::is_valid(m_handle));
        return *m_handle;
    }
    T *operator->() requires std::same_as<handle_type, T *> {
        CheckingPolicy::check(Store::is_valid(m_handle));
        return m_handle;
    }
};

// ============================================================
// SharedOwnership specialization — ref-counted, copyable
// ============================================================

export template <typename T, typename CheckingPolicy,
                 template <typename> typename StoragePolicyT>
class Resource<T, SharedOwnership, CheckingPolicy, StoragePolicyT> {
    using Store     = StoragePolicyT<T>;
    using handle_type = typename Store::handle_type;

    struct ControlBlock {
        handle_type        handle;
        std::atomic<int>   count{1};
        explicit ControlBlock(handle_type h) : handle(h) {}
    };

    ControlBlock *m_ctrl;

    void dec_ref() noexcept {
        if (m_ctrl && m_ctrl->count.fetch_sub(1) == 1) {
            Store::destroy(m_ctrl->handle);
            delete m_ctrl;
            m_ctrl = nullptr;
        }
    }

  public:
    explicit Resource(handle_type h = Store::null_handle())
        : m_ctrl(Store::is_valid(h) ? new ControlBlock{h} : nullptr) {
        CheckingPolicy::check(Store::is_valid(h));
    }

    ~Resource() { dec_ref(); }

    Resource(const Resource &other) : m_ctrl(other.m_ctrl) {
        if (m_ctrl)
            m_ctrl->count.fetch_add(1);
    }
    Resource &operator=(const Resource &other) {
        if (this != &other) {
            dec_ref();
            m_ctrl = other.m_ctrl;
            if (m_ctrl)
                m_ctrl->count.fetch_add(1);
        }
        return *this;
    }

    Resource(Resource &&other) noexcept : m_ctrl(other.m_ctrl) { other.m_ctrl = nullptr; }
    Resource &operator=(Resource &&other) noexcept {
        if (this != &other) {
            dec_ref();
            m_ctrl       = other.m_ctrl;
            other.m_ctrl = nullptr;
        }
        return *this;
    }

    handle_type get() const {
        handle_type h = m_ctrl ? m_ctrl->handle : Store::null_handle();
        CheckingPolicy::check(Store::is_valid(h));
        return h;
    }
    bool valid()     const noexcept { return m_ctrl != nullptr; }
    int  use_count() const noexcept { return m_ctrl ? m_ctrl->count.load() : 0; }

    T &operator*() requires std::same_as<handle_type, T *> {
        auto h = m_ctrl ? m_ctrl->handle : Store::null_handle();
        CheckingPolicy::check(Store::is_valid(h));
        return *h;
    }
    T *operator->() requires std::same_as<handle_type, T *> {
        auto h = m_ctrl ? m_ctrl->handle : Store::null_handle();
        CheckingPolicy::check(Store::is_valid(h));
        return h;
    }
};

// ============================================================
// IntrusiveOwnership specialization — object manages its own ref count
// ============================================================

export template <typename T>
concept IntrusiveCounted = requires(T &t) {
    t.add_ref();
    { t.release()   } -> std::convertible_to<int>;
    { t.ref_count() } -> std::convertible_to<int>;
};

export template <typename T, typename CheckingPolicy,
                 template <typename> typename StoragePolicyT>
    requires IntrusiveCounted<T> &&
             std::same_as<typename StoragePolicyT<T>::handle_type, T *>
class Resource<T, IntrusiveOwnership, CheckingPolicy, StoragePolicyT> {
    using Store     = StoragePolicyT<T>;
    using handle_type = typename Store::handle_type;

    handle_type m_handle;

  public:
    explicit Resource(handle_type h = nullptr) : m_handle(h) {
        CheckingPolicy::check(Store::is_valid(m_handle));
        if (m_handle)
            m_handle->add_ref();
    }

    ~Resource() {
        if (m_handle && m_handle->release() == 0)
            Store::destroy(m_handle);
    }

    Resource(const Resource &other) : m_handle(other.m_handle) {
        if (m_handle)
            m_handle->add_ref();
    }
    Resource &operator=(const Resource &other) {
        if (this != &other) {
            if (m_handle && m_handle->release() == 0)
                Store::destroy(m_handle);
            m_handle = other.m_handle;
            if (m_handle)
                m_handle->add_ref();
        }
        return *this;
    }

    handle_type get() const {
        CheckingPolicy::check(Store::is_valid(m_handle));
        return m_handle;
    }
    bool valid()     const noexcept { return Store::is_valid(m_handle); }
    int  use_count() const noexcept { return m_handle ? m_handle->ref_count() : 0; }

    T &operator*() {
        CheckingPolicy::check(Store::is_valid(m_handle));
        return *m_handle;
    }
    T *operator->() {
        CheckingPolicy::check(Store::is_valid(m_handle));
        return m_handle;
    }
};
