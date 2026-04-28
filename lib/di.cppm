module;

#include <utility>

export module loki23:di;

import :hierarchy;

export {

  // Container inherits the linear hierarchy and adds bind/get/resolve.
  template <typename... Ts>
  struct Container : GenLinearHierarchy<LazyHolder, type_list<Ts...>> {
    using Base = GenLinearHierarchy<LazyHolder, type_list<Ts...>>;

    template <typename X>
    auto resolve(From<X>) { return dep(Field<X>(static_cast<Base &>(*this))); }

    template <typename A>
    decltype(auto) resolve(A &&a) { return std::forward<A>(a); }

    template <typename T, typename... Args>
    void bind(Args &&...args) {
      Field<T>(static_cast<Base &>(*this))
          .bind(resolve(std::forward<Args>(args))...);
    }

    template <typename T>
    T &get() { return Field<T>(static_cast<Base &>(*this)).get(); }
  };

}
