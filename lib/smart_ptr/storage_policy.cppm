module;

#include <mutex>

export module loki23.smart_ptr.storage;

export {
  template <typename Policy, typename Tptr>
  concept StoragePolicy = requires(Policy policy) {
    typename Policy::Tval;
    typename Policy::Tptr;

    { policy.get() } -> std::same_as<typename Policy::Tptr>;
  };

  template <typename Tval_, typename Tptr_ = Tval_ *> struct DefaultStorage {
    using Tval = Tval_;
    using Tptr = Tptr_;

    Tptr ptr_;

    Tptr get() { return ptr_; }
    const Tptr get() const { return ptr_; }

    void set(Tptr ptr) { ptr_ = ptr; }

    constexpr DefaultStorage() : ptr_(nullptr) {}
  };

  template <typename Tval_, typename Tptr_ = Tval_ *> struct LockedStorage {
    using Tval = Tval_;
    using Tptr = Tptr_;

    Tptr ptr_;
    mutable std::mutex mutex_;

    Tptr &get() const {
      std::lock_guard lock(mutex_);
      return ptr_;
    }
  };
}