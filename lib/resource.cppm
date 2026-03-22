module;

#include <cassert>
#include <concepts>
#include <stdexcept>
#include <unistd.h>

export module loki23.resource;

import loki23.smart_ptr.ownership;
import loki23.smart_ptr.storage;

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

export template <typename T>
struct PointerStorage : DefaultStorage<T> {
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
        if (h >= 0) ::close(h);
    }
};

export template <typename T>
struct SocketStorage {
    using handle_type = int;
    static constexpr handle_type null_handle() noexcept { return -1; }
    static bool is_valid(handle_type h) noexcept { return h >= 0; }
    static void destroy(handle_type h) noexcept {
        if (h >= 0) ::close(h);
    }
};

export struct UniqueOwnership {
    UniqueOwnership() = default;
    UniqueOwnership(const UniqueOwnership &) = delete;
    UniqueOwnership &operator=(const UniqueOwnership &) = delete;
    UniqueOwnership(UniqueOwnership &&) noexcept = default;
    UniqueOwnership &operator=(UniqueOwnership &&) noexcept = default;

    template <typename Tptr>
    void acquire(const Tptr &) noexcept {}

    template <typename Tptr>
    bool release(const Tptr &) noexcept { return true; }

    template <typename Tptr>
    int use_count(const Tptr &) const noexcept { return 1; }
};

export struct SharedOwnership : RefCountBase {
    SharedOwnership() : RefCountBase(nullptr) {}

    SharedOwnership(const SharedOwnership &rhs) : RefCountBase(rhs) {}
    SharedOwnership &operator=(const SharedOwnership &rhs) {
        RefCountBase::operator=(rhs); return *this;
    }
    SharedOwnership(SharedOwnership &&rhs) noexcept : RefCountBase(std::move(rhs)) {}
    SharedOwnership &operator=(SharedOwnership &&rhs) noexcept {
        RefCountBase::operator=(std::move(rhs)); return *this;
    }

    template <typename Tptr>
    void acquire(const Tptr &) { alloc_count(); }

    template <typename Tptr>
    Tptr clone(const Tptr &h) noexcept { inc_count(); return h; }

    template <typename Tptr>
    bool release(const Tptr &) noexcept { return dec_count(); }

    template <typename Tptr>
    int use_count(const Tptr &) const noexcept {
        return static_cast<int>(RefCountBase::use_count());
    }
};
export struct IntrusiveOwnership {
    IntrusiveOwnership() = default;

    template <typename Tptr>
    void acquire(const Tptr &h) noexcept { if (h) h->add_ref(); }

    template <typename Tptr>
    Tptr clone(const Tptr &h) noexcept { if (h) h->add_ref(); return h; }

    template <typename Tptr>
    bool release(const Tptr &h) noexcept { return h && h->release() == 0; }

    template <typename Tptr>
    int use_count(const Tptr &h) const noexcept { return h ? h->ref_count() : 0; }
};

export template <typename T>
concept IntrusiveCounted = requires(T &t) {
    t.add_ref();
    { t.release()   } -> std::convertible_to<int>;
    { t.ref_count() } -> std::convertible_to<int>;
};

export template <typename T, typename OwnershipPolicy, typename CheckingPolicy,
                 template <typename> typename StoragePolicyT>
    requires (!std::same_as<OwnershipPolicy, IntrusiveOwnership> ||
              (IntrusiveCounted<T> &&
               std::same_as<typename StoragePolicyT<T>::handle_type, T *>))
class Resource {
    using Store       = StoragePolicyT<T>;
    using handle_type = typename Store::handle_type;

    handle_type     m_handle;
    OwnershipPolicy m_ownership;

  public:
    explicit Resource(handle_type h = Store::null_handle())
        : m_handle(h), m_ownership{} {
        if (Store::is_valid(h))
            m_ownership.acquire(h);
        CheckingPolicy::check(Store::is_valid(h));
    }

    ~Resource() {
        if (Store::is_valid(m_handle) && m_ownership.release(m_handle))
            Store::destroy(m_handle);
    }

    Resource(const Resource &other)
        requires requires(OwnershipPolicy p, handle_type h) { p.clone(h); }
        : m_handle(other.m_handle), m_ownership(other.m_ownership) {
        if (Store::is_valid(m_handle))
            m_ownership.clone(m_handle);
    }

    Resource &operator=(const Resource &other)
        requires requires(OwnershipPolicy p, handle_type h) { p.clone(h); }
    {
        if (this != &other) {
            if (Store::is_valid(m_handle) && m_ownership.release(m_handle))
                Store::destroy(m_handle);
            m_ownership = other.m_ownership;
            m_handle    = m_ownership.clone(other.m_handle);
        }
        return *this;
    }

    Resource(Resource &&other) noexcept
        : m_handle(other.m_handle), m_ownership(std::move(other.m_ownership)) {
        other.m_handle = Store::null_handle();
    }

    Resource &operator=(Resource &&other) noexcept {
        if (this != &other) {
            if (Store::is_valid(m_handle) && m_ownership.release(m_handle))
                Store::destroy(m_handle);
            m_handle    = other.m_handle;
            m_ownership = std::move(other.m_ownership);
            other.m_handle = Store::null_handle();
        }
        return *this;
    }

    handle_type get() const {
        CheckingPolicy::check(Store::is_valid(m_handle));
        return m_handle;
    }

    bool valid()     const noexcept { return Store::is_valid(m_handle); }
    int  use_count() const noexcept { return m_ownership.use_count(m_handle); }

    T &operator*() requires std::same_as<handle_type, T *> {
        CheckingPolicy::check(Store::is_valid(m_handle));
        return *m_handle;
    }
    T *operator->() requires std::same_as<handle_type, T *> {
        CheckingPolicy::check(Store::is_valid(m_handle));
        return m_handle;
    }
};
