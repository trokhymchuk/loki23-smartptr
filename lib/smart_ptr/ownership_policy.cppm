module;

#include <atomic>
#include <cassert>
#include <cstddef>
#include <list>
#include <new>
#include <utility>

export module loki23.smart_ptr.ownership;

namespace detail {
  struct CountBlock {
    std::size_t value;
    CountBlock *next;
  };

  class CountBlockPool {
    CountBlock *free_{nullptr};

  public:
    CountBlockPool() = default;
    CountBlockPool(const CountBlockPool &) = delete;
    CountBlockPool &operator=(const CountBlockPool &) = delete;

    ~CountBlockPool() {
      while (free_) {
        CountBlock *block = free_;
        free_ = free_->next;
        ::operator delete(block);
      }
    }

    std::size_t *allocate() {
      CountBlock *block = free_;
      if (block) {
        free_ = block->next;
      } else {
        block = static_cast<CountBlock *>(::operator new(sizeof(CountBlock)));
      }
      block->value = 1;
      block->next = nullptr;
      return &block->value;
    }

    void deallocate(std::size_t *count) noexcept {
      CountBlock *block = reinterpret_cast<CountBlock *>(count);
      block->next = free_;
      free_ = block;
    }
  };

  inline CountBlockPool count_block_pool;
}

export {

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

    void alloc_count() { count_ = detail::count_block_pool.allocate(); assert(count_); }
    void inc_count()   noexcept { if (count_) ++(*count_); }
    bool dec_count()   noexcept {
      if (!count_) return false;
      if (!--*count_) { detail::count_block_pool.deallocate(count_); count_ = nullptr; return true; }
      return false;
    }

  public:
    std::size_t use_count() const noexcept { return count_ ? *count_ : 0; }
  };

  template <typename Policy, typename Thandle>
  concept Clonable = requires(Policy policy, Thandle h) {
    { policy.clone(h) } -> std::same_as<Thandle>;
  };

  template <typename Policy, typename Thandle>
  concept OwnershipPolicy = requires(Policy policy, Thandle h) {
    { policy.acquire(h) } -> std::same_as<void>;
    { policy.release(h) } -> std::same_as<bool>;
  };

  template <typename Tval>
  concept DeepCopyable = requires(const Tval &v) {
    { v.deep_copy() } -> std::same_as<void>;
  };

  struct RefCounted : RefCountBase {
    RefCounted() : RefCountBase(nullptr) {}
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

  struct RefLinked {
    std::list<RefLinked *> list_;

    RefLinked() : list_{this} {}
    RefLinked(const RefLinked &rhs) : list_(rhs.list_) {}

    template <typename Tptr>
    void acquire(const Tptr &) noexcept {}

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

  template <typename T>
  concept IntrusiveCounted = requires(T &t) {
    t.add_ref();
    { t.release()   } -> std::convertible_to<int>;
    { t.ref_count() } -> std::convertible_to<int>;
  };

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
