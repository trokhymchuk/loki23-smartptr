module;

#include <concepts>
#include <cstddef>
#include <memory>
#include <mutex>
#include <new>
#include <type_traits>
#include <utility>
#ifndef _WIN32
#include <unistd.h>
#endif

export module loki23.smart_ptr.storage;

export {

  template <typename Policy>
  concept StoragePolicy = requires(Policy p, typename Policy::handle_type h) {
    typename Policy::handle_type;
    { p.get() } -> std::same_as<typename Policy::handle_type>;
    { p.set(h) } -> std::same_as<void>;
    { Policy::null_handle() } -> std::same_as<typename Policy::handle_type>;
    { Policy::is_valid(h) } -> std::same_as<bool>;
    { p.destroy(h) } -> std::same_as<void>;
  };

  template <typename Store, typename T>
  concept InitializableStorage = requires(Store s, T val) {
    s.init(std::move(val));
  };

  template <typename T> struct DefaultStorage {
    using handle_type = T *;

    handle_type ptr_{nullptr};

    handle_type get() const noexcept { return ptr_; }
    void set(handle_type h) noexcept { ptr_ = h; }

    static constexpr handle_type null_handle() noexcept { return nullptr; }
    static bool is_valid(handle_type h) noexcept { return h != nullptr; }
    void destroy(handle_type h) noexcept { delete h; }

    template <typename... Args>
        requires std::constructible_from<T, Args...>
    void init(Args &&...args) {
      ptr_ = new T(std::forward<Args>(args)...);
    }
  };

  template <typename T> using PointerStorage = DefaultStorage<T>;

  template <typename T> struct LockedStorage {
    using handle_type = T *;

    mutable handle_type ptr_{nullptr};
    mutable std::mutex mutex_;

    handle_type get() const {
      std::lock_guard lock(mutex_);
      return ptr_;
    }
    void set(handle_type h) {
      std::lock_guard lock(mutex_);
      ptr_ = h;
    }

    static constexpr handle_type null_handle() noexcept { return nullptr; }
    static bool is_valid(handle_type h) noexcept { return h != nullptr; }
    void destroy(handle_type h) noexcept { delete h; }

    template <typename... Args>
        requires std::constructible_from<T, Args...>
    void init(Args &&...args) {
      std::lock_guard lock(mutex_);
      ptr_ = new T(std::forward<Args>(args)...);
    }
  };

#ifndef _WIN32
  template <typename T> struct FileDescriptorStorage {
    using handle_type = int;
    handle_type handle_{-1};

    handle_type get() const noexcept { return handle_; }
    void set(handle_type h) noexcept { handle_ = h; }

    static constexpr handle_type null_handle() noexcept { return -1; }
    static bool is_valid(handle_type h) noexcept { return h >= 0; }
    void destroy(handle_type h) noexcept {
      if (h >= 0)
        ::close(h);
    }
  };

  template <typename T> struct SocketStorage {
    using handle_type = int;
    handle_type handle_{-1};

    handle_type get() const noexcept { return handle_; }
    void set(handle_type h) noexcept { handle_ = h; }

    static constexpr handle_type null_handle() noexcept { return -1; }
    static bool is_valid(handle_type h) noexcept { return h >= 0; }
    void destroy(handle_type h) noexcept {
      if (h >= 0)
        ::close(h);
    }
  };
#endif

  template <typename T, std::size_t N = sizeof(void *) * 4,
            typename Allocator = std::allocator<std::byte>>
  struct SmallBufferStorage {
    static constexpr bool fits_inline =
        sizeof(T) <= N && alignof(T) <= alignof(std::max_align_t);
    using handle_type = T *;

    alignas(std::max_align_t) std::byte buffer_[N];
    handle_type ptr_{nullptr};
    [[no_unique_address]] Allocator allocator_;

    SmallBufferStorage() = default;

    SmallBufferStorage(SmallBufferStorage &&rhs) noexcept
        : allocator_(std::move(rhs.allocator_)) {
      if (rhs.ptr_) {
        if constexpr (fits_inline) {
          ::new (buffer_) T(std::move(*rhs.ptr_));
          rhs.ptr_->~T();
          ptr_ = std::launder(reinterpret_cast<T *>(buffer_));
        } else {
          ptr_ = rhs.ptr_;
        }
        rhs.ptr_ = nullptr;
      }
    }

    SmallBufferStorage &operator=(SmallBufferStorage &&rhs) noexcept {
      if (this != &rhs) {
        if (ptr_)
          destroy(ptr_);
        ptr_ = nullptr;
        if (rhs.ptr_) {
          if constexpr (fits_inline) {
            ::new (buffer_) T(std::move(*rhs.ptr_));
            rhs.ptr_->~T();
            ptr_ = std::launder(reinterpret_cast<T *>(buffer_));
          } else {
            ptr_ = rhs.ptr_;
          }
          rhs.ptr_ = nullptr;
        }
        allocator_ = std::move(rhs.allocator_);
      }
      return *this;
    }

    SmallBufferStorage(const SmallBufferStorage &) = delete;
    SmallBufferStorage &operator=(const SmallBufferStorage &) = delete;

    handle_type get() const noexcept { return ptr_; }
    void set(handle_type h) noexcept { ptr_ = h; }

    static constexpr handle_type null_handle() noexcept { return nullptr; }
    static bool is_valid(handle_type h) noexcept { return h != nullptr; }

    void destroy(handle_type h) {
      using TAlloc =
          typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
      TAlloc alloc(allocator_);
      std::allocator_traits<TAlloc>::destroy(alloc, h);
      if constexpr (!fits_inline) {
        std::allocator_traits<TAlloc>::deallocate(alloc, h, 1);
      }
    }

    template <typename... Args>
        requires std::constructible_from<T, Args...>
    void init(Args &&...args) {
      using TAlloc =
          typename std::allocator_traits<Allocator>::template rebind_alloc<T>;
      TAlloc alloc(allocator_);
      T *mem;
      if constexpr (fits_inline) {
        mem = reinterpret_cast<T *>(buffer_);
      } else {
        mem = std::allocator_traits<TAlloc>::allocate(alloc, 1);
      }
      std::allocator_traits<TAlloc>::construct(alloc, mem,
                                               std::forward<Args>(args)...);
      ptr_ = mem;
    }
  };

  template <typename T> using SmallBuffer = SmallBufferStorage<T>;
}
