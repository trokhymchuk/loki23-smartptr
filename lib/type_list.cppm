module;

#include <cstddef>
#include <type_traits>

export module loki23:type_list;

export template <typename... T> struct type_list;

#if 0
export template <typename T, typename... Types> struct type_in_list;

export template <typename T, typename E, typename... Types> struct type_in_list<T, type_list<E, Types...>> {
  static constexpr bool value{(std::is_same<T, E>::value) || type_in_list<T, type_list<Types...>>::value};
};

export template <typename T> struct type_in_list<T, type_list<>> {
  static constexpr bool value{};
};
#endif

export template <typename... T> struct type_list {
#if 0
  template <typename E, typename... M>
    requires std::is_constructible_v<E, M...>
  [[nodiscard]]
  std::enable_if<type_in_list<E, type_list<T...>>::value, E *>::type create(M... args) const {
    return new E(args...);
  }
#endif
};

#if 0
export template <typename T, typename E> struct push_type_back;
export template <typename T, typename... E> struct push_type_back<T, type_list<E...>> {
  using type = type_list<E..., T>;
};

export template <typename T, typename E> struct push_type_front;
export template <typename T, typename... E> struct push_type_front<T, type_list<E...>> {
  using type = type_list<T, E...>;
};

export template <std::size_t Index, typename... Types> struct type_at;

export template <std::size_t Index, typename T, typename... Types>
struct type_at<Index, T, Types...> : type_at<Index - 1, Types...> {};

export template <typename T, typename... Types> struct type_at<0, T, Types...> {
  using type = T;
};

export template <typename T, typename... Types> struct type_in_list;

export template <typename T, typename E, typename... Types>
struct type_in_list<T, E, Types...> : type_in_list<T, Types...> {
  bool value{(std::is_same<T, E>::value) || type_in_list<T, Types...>::value};
};

export template <typename T> struct type_in_list<T> {
  bool value{};
};

template <typename T, std::size_t initial, typename E> struct fold_left {
  static constexpr size_t value = init ial;
};
export template <typename T, std::size_t initial, typename E, typename... M>
struct fold_left<T, initial, type_list<E, M...>> {
  static constexpr size_t value = T::f(initial, sizeof(E)) + fold_left<T, initial, type_list<M...>>::value;
};

export struct sum_size_t {
  static constexpr std::size_t f(std::size_t lhs, std::size_t rhs) { return lhs + rhs; }
};

export template <template <typename T> typename E, typename... M> void do_work(type_list<M...>) { (E<M>::work(), ...); }
#endif