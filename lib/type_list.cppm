module;

#include <cstddef>
#include <type_traits>

export module loki23:type_list;

export template <typename... Ts>
struct type_list {};

// ── length<TL>────────────
export template <typename TL>
struct length;

export template <typename... Ts>
struct length<type_list<Ts...>>
    : std::integral_constant<std::size_t, sizeof...(Ts)> {};

export template <typename TL>
inline constexpr std::size_t length_v = length<TL>::value;

// ── type_at<I, TL>────────
export template <std::size_t I, typename TL>
struct type_at;

export template <std::size_t I, typename T, typename... Ts>
struct type_at<I, type_list<T, Ts...>> : type_at<I - 1, type_list<Ts...>> {};

export template <typename T, typename... Ts>
struct type_at<0, type_list<T, Ts...>> {
    using type = T;
};

export template <std::size_t I, typename TL>
using type_at_t = typename type_at<I, TL>::type;

// ── type_at_non_strict<I, TL, Default> ───────────────────────────────────────

export template <std::size_t I, typename TL, typename Default = void>
struct type_at_non_strict {
    using type = Default;
};

export template <std::size_t I, typename T, typename... Ts, typename Default>
struct type_at_non_strict<I, type_list<T, Ts...>, Default>
    : type_at_non_strict<I - 1, type_list<Ts...>, Default> {};

export template <typename T, typename... Ts, typename Default>
struct type_at_non_strict<0, type_list<T, Ts...>, Default> {
    using type = T;
};

export template <std::size_t I, typename TL, typename Default = void>
using type_at_non_strict_t = typename type_at_non_strict<I, TL, Default>::type;

// ── index_of<T, TL>───────

export template <typename T, typename TL>
struct index_of;

export template <typename T>
struct index_of<T, type_list<>>
    : std::integral_constant<int, -1> {};

export template <typename T, typename... Ts>
struct index_of<T, type_list<T, Ts...>>
    : std::integral_constant<int, 0> {};

export template <typename T, typename U, typename... Ts>
struct index_of<T, type_list<U, Ts...>> {
private:
    static constexpr int tail = index_of<T, type_list<Ts...>>::value;
public:
    static constexpr int value = tail == -1 ? -1 : 1 + tail;
};

export template <typename T, typename TL>
inline constexpr int index_of_v = index_of<T, TL>::value;

// ── contains<T, TL>───────

export template <typename T, typename TL>
struct contains : std::bool_constant<index_of_v<T, TL> != -1> {};

export template <typename T, typename TL>
inline constexpr bool contains_v = contains<T, TL>::value;

// ── push_front<T, TL>─────

export template <typename T, typename TL>
struct push_front;

export template <typename T, typename... Ts>
struct push_front<T, type_list<Ts...>> {
    using type = type_list<T, Ts...>;
};

export template <typename T, typename TL>
using push_front_t = typename push_front<T, TL>::type;

// ── push_back<T, TL>──────

export template <typename T, typename TL>
struct push_back;

export template <typename T, typename... Ts>
struct push_back<T, type_list<Ts...>> {
    using type = type_list<Ts..., T>;
};

export template <typename T, typename TL>
using push_back_t = typename push_back<T, TL>::type;

// ── append<TL1, TL2>──────

export template <typename TL1, typename TL2>
struct append;

export template <typename... Ts, typename... Us>
struct append<type_list<Ts...>, type_list<Us...>> {
    using type = type_list<Ts..., Us...>;
};

export template <typename TL1, typename TL2>
using append_t = typename append<TL1, TL2>::type;

// ── erase<T, TL>──────────

export template <typename T, typename TL>
struct erase;

export template <typename T>
struct erase<T, type_list<>> {
    using type = type_list<>;
};

export template <typename T, typename... Ts>
struct erase<T, type_list<T, Ts...>> {
    using type = type_list<Ts...>;
};

export template <typename T, typename U, typename... Ts>
struct erase<T, type_list<U, Ts...>> {
    using type = push_front_t<U, typename erase<T, type_list<Ts...>>::type>;
};

export template <typename T, typename TL>
using erase_t = typename erase<T, TL>::type;

// ── erase_all<T, TL>──────

export template <typename T, typename TL>
struct erase_all;

export template <typename T>
struct erase_all<T, type_list<>> {
    using type = type_list<>;
};

export template <typename T, typename... Ts>
struct erase_all<T, type_list<T, Ts...>> {
    using type = typename erase_all<T, type_list<Ts...>>::type;
};

export template <typename T, typename U, typename... Ts>
struct erase_all<T, type_list<U, Ts...>> {
    using type = push_front_t<U, typename erase_all<T, type_list<Ts...>>::type>;
};

export template <typename T, typename TL>
using erase_all_t = typename erase_all<T, TL>::type;

// ── no_duplicates<TL>─────

export template <typename TL>
struct no_duplicates;

export template <>
struct no_duplicates<type_list<>> {
    using type = type_list<>;
};

export template <typename T, typename... Ts>
struct no_duplicates<type_list<T, Ts...>> {
private:
    using unique_tail = typename no_duplicates<type_list<Ts...>>::type;
public:
    using type = push_front_t<T, erase_all_t<T, unique_tail>>;
};

export template <typename TL>
using no_duplicates_t = typename no_duplicates<TL>::type;

// ── replace<T, U, TL>─────

export template <typename T, typename U, typename TL>
struct replace;

export template <typename T, typename U>
struct replace<T, U, type_list<>> {
    using type = type_list<>;
};

export template <typename T, typename U, typename... Ts>
struct replace<T, U, type_list<T, Ts...>> {
    using type = type_list<U, Ts...>;
};

export template <typename T, typename U, typename V, typename... Ts>
struct replace<T, U, type_list<V, Ts...>> {
    using type = push_front_t<V, typename replace<T, U, type_list<Ts...>>::type>;
};

export template <typename T, typename U, typename TL>
using replace_t = typename replace<T, U, TL>::type;

// ── replace_all<T, U, TL>─

export template <typename T, typename U, typename TL>
struct replace_all;

export template <typename T, typename U>
struct replace_all<T, U, type_list<>> {
    using type = type_list<>;
};

export template <typename T, typename U, typename... Ts>
struct replace_all<T, U, type_list<T, Ts...>> {
    using type = push_front_t<U, typename replace_all<T, U, type_list<Ts...>>::type>;
};

export template <typename T, typename U, typename V, typename... Ts>
struct replace_all<T, U, type_list<V, Ts...>> {
    using type = push_front_t<V, typename replace_all<T, U, type_list<Ts...>>::type>;
};

export template <typename T, typename U, typename TL>
using replace_all_t = typename replace_all<T, U, TL>::type;

// ── reverse<TL>───────────

export template <typename TL>
struct reverse;

export template <>
struct reverse<type_list<>> {
    using type = type_list<>;
};

export template <typename T, typename... Ts>
struct reverse<type_list<T, Ts...>> {
    using type = push_back_t<T, typename reverse<type_list<Ts...>>::type>;
};

export template <typename TL>
using reverse_t = typename reverse<TL>::type;

// ── most_derived<TL, Base>

export template <typename TL, typename Base>
struct most_derived;

export template <typename Base>
struct most_derived<type_list<>, Base> {
    using type = Base;
};

export template <typename T, typename... Ts, typename Base>
struct most_derived<type_list<T, Ts...>, Base> {
private:
    using candidate = typename most_derived<type_list<Ts...>, Base>::type;
public:
    using type = std::conditional_t<std::is_base_of_v<candidate, T>, T, candidate>;
};

export template <typename TL, typename Base>
using most_derived_t = typename most_derived<TL, Base>::type;

// ── derived_to_front<TL>──

export template <typename TL>
struct derived_to_front;

export template <>
struct derived_to_front<type_list<>> {
    using type = type_list<>;
};

export template <typename T, typename... Ts>
struct derived_to_front<type_list<T, Ts...>> {
private:
    using most  = most_derived_t<type_list<T, Ts...>, T>;
    using rest  = erase_t<most, type_list<T, Ts...>>;
    using sorted = typename derived_to_front<rest>::type;
public:
    using type = push_front_t<most, sorted>;
};

export template <typename TL>
using derived_to_front_t = typename derived_to_front<TL>::type;

// ── apply<F, TL>──────────

export template <template <typename> typename F, typename TL>
struct apply;

export template <template <typename> typename F, typename... Ts>
struct apply<F, type_list<Ts...>> {
    using type = type_list<typename F<Ts>::type...>;
};

export template <template <typename> typename F, typename TL>
using apply_t = typename apply<F, TL>::type;

// ── filter<Pred, TL>──────

export template <template <typename> typename Pred, typename TL>
struct filter;

export template <template <typename> typename Pred>
struct filter<Pred, type_list<>> {
    using type = type_list<>;
};

export template <template <typename> typename Pred, typename T, typename... Ts>
struct filter<Pred, type_list<T, Ts...>> {
private:
    using tail = typename filter<Pred, type_list<Ts...>>::type;
public:
    using type = std::conditional_t<Pred<T>::value, push_front_t<T, tail>, tail>;
};

export template <template <typename> typename Pred, typename TL>
using filter_t = typename filter<Pred, TL>::type;

// ── for_each<TL>(f)───────

export template <typename... Ts, typename F>
constexpr void for_each(type_list<Ts...>, F &&f) {
    (f.template operator()<Ts>(), ...);
}
