module;

#include <atomic>
#include <cassert>
#include <cstddef>
#include <list>
#include <utility>

export module loki23.smart_ptr.ownership;

export {

  // ============================================================
  // Base class for ref-counted ownership
  // ============================================================

  class RefCountBase {
  protected:
    std::size_t *count_;

    explicit RefCountBase(std::nullptr_t) noexcept : count_(nullptr) {}

    RefCountBase(const RefCountBase &rhs) noexcept : count_(rhs.count_) {}
    RefCountBase &operator=(const RefCountBase &rhs) noexcept { count_ = rhs.count_; return *this; }

    RefCountBase(RefCountBase &&rhs) noexcept : count_(rhs.count_) { rhs.count_ = nullptr; }
    RefCountBase &operator=(RefCountBase &&rhs) noexcept {
      count_ = rhs.count_; rhs.count_ = nullptr; return *this;
    }

    void alloc_count() { count_ = new std::size_t{1}; assert(count_); }
    void inc_count()   noexcept { if (count_) ++(*count_); }
    bool dec_count()   noexcept {
      if (!count_) return false;
      if (!--*count_) { delete count_; count_ = nullptr; return true; }
      return false;
    }

  public:
    std::size_t use_count() const noexcept { return count_ ? *count_ : 0; }
  };

  // ============================================================
  // Concepts
  // ============================================================

  template <typename Policy, typename Thandle>
  concept Clonable = requires(Policy policy, Thandle h) {
    { policy.clone(h) } -> std::same_as<Thandle>;
  };

  template <typename Policy, typename Thandle>
  concept OwnershipPolicy = requires(Policy policy, Thandle h) {
    { policy.acquire(h) } -> std::same_as<void>;
    { policy.release(h) } -> std::same_as<bool>;
  };

  // Misc concept
  template <typename Tval>
  concept DeepCopyable = requires(const Tval &v) {
    { v.deep_copy() } -> std::same_as<void>;
  };

  // ============================================================
  // SmartPtr ownership policies (non-template, template methods)
  // ============================================================

  // Ref-counted — shared ownership via heap-allocated counter
  struct RefCounted : RefCountBase {
    RefCounted() : RefCountBase(nullptr) {}  // no alloc: acquire() does it
    RefCounted(const RefCounted &rhs) : RefCountBase(rhs) {}
    RefCounted &operator=(const RefCounted &) = default;

    template <typename Tptr>
    void acquire(const Tptr &) { alloc_count(); }

    template <typename Tptr>
    Tptr clone(const Tptr &h) { inc_count(); return h; }

    template <typename Tptr>
    bool release(const Tptr &) { return dec_count(); }

    template <typename Tptr>
    int use_count(const Tptr &) const noexcept {
      return static_cast<int>(RefCountBase::use_count());
    }
  };

  // Thread-safe ref-counted base — atomic heap-allocated counter
  class AtomicRefCountBase {
  protected:
    std::atomic<std::size_t> *count_;

    explicit AtomicRefCountBase(std::nullptr_t) noexcept : count_(nullptr) {}

    AtomicRefCountBase(const AtomicRefCountBase &rhs) noexcept : count_(rhs.count_) {}
    AtomicRefCountBase &operator=(const AtomicRefCountBase &rhs) noexcept { count_ = rhs.count_; return *this; }

    AtomicRefCountBase(AtomicRefCountBase &&rhs) noexcept : count_(rhs.count_) { rhs.count_ = nullptr; }
    AtomicRefCountBase &operator=(AtomicRefCountBase &&rhs) noexcept {
      count_ = rhs.count_; rhs.count_ = nullptr; return *this;
    }

    void alloc_count() { count_ = new std::atomic<std::size_t>{1}; assert(count_); }

    void inc_count() noexcept {
      if (count_) count_->fetch_add(1, std::memory_order_relaxed);
    }

    bool dec_count() noexcept {
      if (!count_) return false;
      if (count_->fetch_sub(1, std::memory_order_acq_rel) == 1) {
        delete count_; count_ = nullptr; return true;
      }
      return false;
    }

  public:
    std::size_t use_count() const noexcept {
      return count_ ? count_->load(std::memory_order_relaxed) : 0;
    }
  };

  // Thread-safe ref-counted — shared ownership via atomic heap-allocated counter
  struct RefCountedMT : AtomicRefCountBase {
    RefCountedMT() : AtomicRefCountBase(nullptr) {}
    RefCountedMT(const RefCountedMT &rhs) : AtomicRefCountBase(rhs) {}
    RefCountedMT &operator=(const RefCountedMT &) = default;

    template <typename Tptr>
    void acquire(const Tptr &) { alloc_count(); }

    template <typename Tptr>
    Tptr clone(const Tptr &h) { inc_count(); return h; }

    template <typename Tptr>
    bool release(const Tptr &) { return dec_count(); }

    template <typename Tptr>
    int use_count(const Tptr &) const noexcept {
      return static_cast<int>(AtomicRefCountBase::use_count());
    }
  };

  // Linked-list ref counting — membership tracked per-instance
  struct RefLinked {
    std::list<RefLinked *> list_;

    RefLinked() : list_{this} {}
    RefLinked(const RefLinked &rhs) : list_(rhs.list_) {}

    template <typename Tptr>
    void acquire(const Tptr &) noexcept {}  // list seeded in constructor

    template <typename Tptr>
    Tptr clone(const Tptr &v) { list_.push_back(this); return v; }

    template <typename Tptr>
    bool release(const Tptr &) {
      list_.remove(this);
      return list_.empty();
    }

    template <typename Tptr>
    int use_count(const Tptr &) const noexcept {
      return static_cast<int>(list_.size());
    }
  };

  // Shallow copy — pointer is shared, each owner destroys independently
  struct Copy {
    template <typename Tptr>
    void acquire(const Tptr &) noexcept {}

    template <typename Tptr>
    Tptr clone(const Tptr &v) { return v; }

    template <typename Tptr>
    bool release(const Tptr &) noexcept { return true; }

    template <typename Tptr>
    int use_count(const Tptr &) const noexcept { return 1; }
  };

  // Deep copy — each owner gets its own independent copy
  struct DeepCopy {
    template <typename Tptr>
    void acquire(const Tptr &) noexcept {}

    template <typename Tptr>
    Tptr clone(const Tptr &v) {
      if (!v) return v;
      using Tval = std::remove_pointer_t<Tptr>;
      if constexpr (DeepCopyable<Tval>) {
        return v->deep_copy();
      } else {
        return new Tval{*v};
      }
    }

    template <typename Tptr>
    bool release(const Tptr &) noexcept { return true; }

    template <typename Tptr>
    int use_count(const Tptr &) const noexcept { return 1; }
  };

  // No copy — move-only ownership
  struct NoCopy {
    template <typename Tptr>
    void acquire(const Tptr &) noexcept {}

    template <typename Tptr>
    Tptr clone(const Tptr &) = delete;

    template <typename Tptr>
    bool release(const Tptr &) noexcept { return true; }

    template <typename Tptr>
    int use_count(const Tptr &) const noexcept { return 1; }
  };

  // ============================================================
  // Resource ownership policies
  // ============================================================

  // Single owner — move-only, always destroys on release
  struct UniqueOwnership {
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

  // Shared owner — ref-counted via RefCountBase
  struct SharedOwnership : RefCountBase {
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

  // Concept for objects that carry their own ref count
  template <typename T>
  concept IntrusiveCounted = requires(T &t) {
    t.add_ref();
    { t.release()   } -> std::convertible_to<int>;
    { t.ref_count() } -> std::convertible_to<int>;
  };

  // Intrusive owner — delegates ref counting to the object itself
  struct IntrusiveOwnership {
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
}
