module;

#include <utility>

export module loki23:di;

import :type_list;
import :hierarchy;

export {

  template <typename T>
  struct From {};

  template <typename... Ts>
  struct Container {
    GenLinearHierarchy<LazyHolder, type_list<Ts...>> m_hierarchy;

    template <typename X>
    auto resolve(From<X>) { return dep(Field<X>(m_hierarchy)); }

    template <typename A>
    decltype(auto) resolve(A &&a) { return std::forward<A>(a); }

    template <typename T, typename... Args>
    void bind(Args &&...args) {
      Field<T>(m_hierarchy).bind(resolve(std::forward<Args>(args))...);
    }

    template <typename T>
    T &get() {
      return Field<T>(m_hierarchy).get();
    }
  };

}
