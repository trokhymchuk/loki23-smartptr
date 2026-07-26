module;

#include <concepts>
#include <type_traits>
#include <utility>

export module loki23.smart_ptr;

export import loki23.smart_ptr.checking;
export import loki23.smart_ptr.conversion;
export import loki23.smart_ptr.ownership;
export import loki23.smart_ptr.storage;

export {
  template <typename Tval,
            typename OwnPolicy   = DeepCopy,
            typename CheckPolicy = NoCheck,
            template <typename> typename StorePolicy = DefaultStorage,
            typename ConvPolicy  = DisallowConversion>
    requires ((!std::same_as<OwnPolicy, IntrusiveOwnership> ||
               (IntrusiveCounted<Tval> &&
                std::same_as<typename StorePolicy<Tval>::handle_type, Tval *>)) &&
              ConversionPolicy<ConvPolicy, typename StorePolicy<Tval>::handle_type>)
  class SmartPtr {
    using Store       = StorePolicy<Tval>;
    using handle_type = typename Store::handle_type;

    Store     storage_{};
    OwnPolicy ownership_{};

    void clean() {
      handle_type h = storage_.get();
      if (Store::is_valid(h) && ownership_.release(h))
        storage_.destroy(h);
    }

  public:
    SmartPtr() = default;

    explicit SmartPtr(handle_type h) {
      storage_.set(h);
      if (Store::is_valid(h))
        ownership_.acquire(h);
      CheckPolicy::check(Store::is_valid(h));
    }

    template <typename... Args>
        requires (InitializableStorage<Store, Tval> &&
                  std::constructible_from<Tval, Args...> &&
                  sizeof...(Args) > 0 &&
                  !(sizeof...(Args) == 1 &&
                    (std::same_as<std::remove_cvref_t<Args>, handle_type> || ...)))
    explicit SmartPtr(Args &&...args) {
      storage_.init(std::forward<Args>(args)...);
      handle_type h = storage_.get();
      ownership_.acquire(h);
      CheckPolicy::check(Store::is_valid(h));
    }

    ~SmartPtr() { clean(); }

    SmartPtr(const SmartPtr &rhs)
        requires Clonable<OwnPolicy, handle_type>
        : ownership_(rhs.ownership_) {
      handle_type h = rhs.storage_.get();
      storage_.set(Store::is_valid(h) ? ownership_.clone(h) : Store::null_handle());
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
      handle_type h = storage_.get();
      CheckPolicy::check(Store::is_valid(h));
      return h;
    }

    operator handle_type() const requires (ConvPolicy::implicit_convert()) {
      handle_type h = storage_.get();
      CheckPolicy::check(Store::is_valid(h));
      return h;
    }
    bool        valid()     const noexcept { return Store::is_valid(storage_.get()); }
    int         use_count() const noexcept { return ownership_.use_count(storage_.get()); }

    Tval &operator*()
        requires std::same_as<handle_type, Tval *>
    {
      handle_type h = storage_.get();
      CheckPolicy::check(Store::is_valid(h));
      return *h;
    }
    Tval *operator->()
        requires std::same_as<handle_type, Tval *>
    {
      handle_type h = storage_.get();
      CheckPolicy::check(Store::is_valid(h));
      return h;
    }
  };
}
