module;

#include <iostream>
#include <cassert>
#include <cstddef>
#include <list>
#include <utility>

export module loki23.smart_ptr.ownership;

export {

  // ============================================================
  // Base class for ref-counted ownership (shared by RefCounted
  // and resource::SharedOwnership)
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

    void inc_count() noexcept { if (count_) ++(*count_); }

    bool dec_count() noexcept {
      if (!count_) return false;
      if (!--*count_) { delete count_; count_ = nullptr; return true; }
      return false;
    }

  public:
    std::size_t use_count() const noexcept { return count_ ? *count_ : 0; }
  };

  // optional
  template <typename Policy, typename Tptr>
  concept Clonable = requires(Policy policy, Tptr ptr) {
    { policy.clone(std::declval<const Tptr &>()) } -> std::same_as<Tptr>;
  };

  // required
  template <typename Policy, typename Tptr>
  concept Releasable = requires(Policy policy, Tptr ptr) {
    { policy.release(ptr) } -> std::same_as<bool>;
  };

  template <typename Policy, typename Tptr>
  concept OwnershipPolicy = Releasable<Policy, Tptr>;

  // Misc
  template <typename Tval>
  concept DeepCopyable = requires(const Tval &v) {
    { v.deep_copy() } -> std::same_as<void>;
  };

  // Policies:
  template <typename Tval, typename Tptr = Tval *>
  class RefCounted : public RefCountBase {
  public:
    RefCounted() : RefCountBase(nullptr) { alloc_count(); }

    RefCounted(const RefCounted &rhs) : RefCountBase(rhs) {}
    RefCounted &operator=(const RefCounted &) = default;

    Tptr clone(const Tptr &val) { inc_count(); return val; }

    bool release(const Tptr &) { return dec_count(); }
  };

  template <typename Tval, typename Tptr = Tval *> class RefLinked {

    std::list<RefLinked *> list_;

  public:
    RefLinked() : list_{} {}

    RefLinked(const RefLinked &rhs) : list_{rhs.list_} {}

    bool release(const Tptr &) {
      if (list_.front() == list_.back() && list_.front() == this) {
        return true;
      }
      return false;
    }
    Tptr clone(const Tptr &v) {
      list_.push_back(this);
      return v;
    }
  };

  template <typename Tval, typename Tptr = Tval *> struct Copy {

    bool release(const Tptr &) { return true; }
    Tptr clone(const Tptr &v) { return v; }
  };

  template <typename Tval, typename Tptr = Tval *> struct DeepCopy {

    bool release(const Tptr &) { return true; }
    Tptr clone(const Tptr &v) {
      if (!v)
        return nullptr;

      if constexpr (DeepCopyable<Tptr>) {
        return v->deep_copy();
      } else {
        return new Tval{*v};
      }
    }
  };

  template <typename Tval, typename Tptr = Tval *> struct NoCopy {

    bool release(const Tptr &) { return true; }
    Tptr clone(const Tptr &v) = delete;
  };
}
