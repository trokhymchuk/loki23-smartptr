module;

#include <concepts>
#include <utility>

export module loki23.smart_ptr;

export import loki23.smart_ptr.checking;
export import loki23.smart_ptr.ownership;
export import loki23.smart_ptr.storage;

export {
  template <typename Tval,
            typename OwnPolicy   = DeepCopy,
            typename CheckPolicy = NoCheck,
            template <typename> typename StorePolicy = DefaultStorage>
    requires (!std::same_as<OwnPolicy, IntrusiveOwnership> ||
              (IntrusiveCounted<Tval> &&
               std::same_as<typename StorePolicy<Tval>::handle_type, Tval *>))
  class SmartPtr {
    using Store       = StorePolicy<Tval>;
    using handle_type = typename Store::handle_type;

    Store     storage_{};
    OwnPolicy ownership_{};

    void clean() {
      if (Store::is_valid(storage_.get()) && ownership_.release(storage_.get()))
        Store::destroy(storage_.get());
    }

  public:
    SmartPtr() = default;

    explicit SmartPtr(handle_type h) {
      storage_.set(h);
      if (Store::is_valid(h))
        ownership_.acquire(h);
      CheckPolicy::check(Store::is_valid(h));
    }

    ~SmartPtr() { clean(); }

    // Copy — only when OwnPolicy provides clone()
    SmartPtr(const SmartPtr &rhs)
        requires Clonable<OwnPolicy, handle_type>
        : ownership_(rhs.ownership_) {
      handle_type h = rhs.storage_.get();
      storage_.set(h);
      if (Store::is_valid(h))
        ownership_.clone(h);
    }

    SmartPtr &operator=(const SmartPtr &rhs)
        requires Clonable<OwnPolicy, handle_type>
    {
      if (this != &rhs) {
        clean();
        ownership_ = rhs.ownership_;
        handle_type h = rhs.storage_.get();
        storage_.set(Store::is_valid(h) ? ownership_.clone(h) : Store::null_handle());
      }
      return *this;
    }

    SmartPtr(SmartPtr &&rhs) noexcept
        : storage_(std::move(rhs.storage_)), ownership_(std::move(rhs.ownership_)) {
      rhs.storage_.set(Store::null_handle());
    }

    SmartPtr &operator=(SmartPtr &&rhs) noexcept {
      if (this != &rhs) {
        clean();
        storage_   = std::move(rhs.storage_);
        ownership_ = std::move(rhs.ownership_);
        rhs.storage_.set(Store::null_handle());
      }
      return *this;
    }

    handle_type get() const {
      CheckPolicy::check(valid());
      return storage_.get();
    }
    bool        valid()     const noexcept { return Store::is_valid(storage_.get()); }
    int         use_count() const noexcept { return ownership_.use_count(storage_.get()); }

    Tval &operator*()
        requires std::same_as<handle_type, Tval *>
    {
      CheckPolicy::check(valid());
      return *storage_.get();
    }
    Tval *operator->()
        requires std::same_as<handle_type, Tval *>
    {
      CheckPolicy::check(valid());
      return storage_.get();
    }
  };
}
