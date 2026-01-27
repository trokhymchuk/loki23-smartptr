module;
#include <iostream>
#include <cassert>

export module loki23.smart_ptr;

export import loki23.smart_ptr.checking;
export import loki23.smart_ptr.conversion;
export import loki23.smart_ptr.ownership;
export import loki23.smart_ptr.storage;

export {
  template <typename Tval, typename Tptr = Tval *, OwnershipPolicy<Tptr> OwnPolicy = DeepCopy<Tval, Tptr>,
            CheckingPolicy<Tptr> CheckPolicy = NoCheck<Tval, Tptr>, ConversionPolicy<Tptr> ConvPolicy = AllowConversion,
            StoragePolicy<Tptr> StorePolicy = DefaultStorage<Tval, Tptr>>
  class SmartPtr {
    CheckPolicy checking_policy_;
    ConvPolicy conversion_policy_;
    OwnPolicy ownership_policy_;
    StorePolicy storage_policy_;

    void clean() {
      if (ownership_policy_.release(get())) {
        delete get();
      }
    }

  public:
    SmartPtr() : checking_policy_{}, conversion_policy_{}, ownership_policy_{}, storage_policy_{} {
      checking_policy_.on_init(Tptr{});
    }
    SmartPtr(const Tptr ptr) : checking_policy_{}, conversion_policy_{}, ownership_policy_{}, storage_policy_{} {
      storage_policy_.ptr_ = ptr;
      checking_policy_.on_init(ptr);
    }
    SmartPtr(const SmartPtr &rhs)
      requires Clonable<OwnPolicy, Tptr>
        : checking_policy_{rhs.checking_policy_}, conversion_policy_{rhs.conversion_policy_},
          ownership_policy_{rhs.ownership_policy_}, storage_policy_{} {
      storage_policy_.set(ownership_policy_.clone(rhs.get()));
    }

    Tval operator*() {
      checking_policy_.on_dereference(get());
      return get();
    }
    Tptr operator->() {
      checking_policy_.on_dereference(get());
      return get();
    }

    SmartPtr &operator=(const SmartPtr &rhs)
      requires Clonable<OwnPolicy, Tptr>
    {
      clean();
      checking_policy_ = rhs.checking_policy_;
      conversion_policy_ = rhs.conversion_policy_;
      ownership_policy_ = rhs.ownership_policy_;
      storage_policy_ = rhs.storage_policy_;

      storage_policy_.set(ownership_policy_.clone(rhs.get()));

      return *this;
    }

    Tptr get() { return storage_policy_.get(); }
    const Tptr get() const { return storage_policy_.get(); }

    ~SmartPtr() { clean(); }
  };
}