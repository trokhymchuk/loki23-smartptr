module;

#include <mutex>

export module loki23.smart_ptr.storage;

export {

  template <typename Policy>
  concept StoragePolicy = requires(Policy p, typename Policy::handle_type h) {
    typename Policy::handle_type;
    { p.get() }              -> std::same_as<typename Policy::handle_type>;
    { p.set(h) }             -> std::same_as<void>;
    { Policy::null_handle() } -> std::same_as<typename Policy::handle_type>;
    { Policy::is_valid(h) }  -> std::same_as<bool>;
    { Policy::destroy(h) }   -> std::same_as<void>;
  };

  // Default heap-pointer storage: holds T*, owns with delete
  template <typename T>
  struct DefaultStorage {
    using handle_type = T *;

    handle_type ptr_{nullptr};

    handle_type       get()           const noexcept { return ptr_; }
    void              set(handle_type h)    noexcept { ptr_ = h; }

    static constexpr handle_type null_handle() noexcept { return nullptr; }
    static bool is_valid(handle_type h)  noexcept { return h != nullptr; }
    static void destroy(handle_type h)   noexcept { delete h; }
  };

  // Semantic alias — PointerStorage and DefaultStorage are identical
  template <typename T>
  using PointerStorage = DefaultStorage<T>;

  // Thread-safe pointer storage (get/set protected by mutex)
  template <typename T>
  struct LockedStorage {
    using handle_type = T *;

    mutable handle_type ptr_{nullptr};
    mutable std::mutex  mutex_;

    handle_type get() const {
      std::lock_guard lock(mutex_);
      return ptr_;
    }
    void set(handle_type h) {
      std::lock_guard lock(mutex_);
      ptr_ = h;
    }

    static constexpr handle_type null_handle() noexcept { return nullptr; }
    static bool is_valid(handle_type h)  noexcept { return h != nullptr; }
    static void destroy(handle_type h)   noexcept { delete h; }
  };
}
