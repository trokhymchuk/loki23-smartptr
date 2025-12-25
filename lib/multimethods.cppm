module;

#include <array>
#include <functional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include <iostream>

export module loki23:multimethods;

import :type_list;

template <typename Holder = void> struct Executor;
template <> struct Executor<void> {
  template <typename T, typename E, typename... Executors> static bool execute(T &lhs, E &rhs, Executors... execs) {
    bool found = (... || ([&]() {
                    if constexpr (std::is_invocable<decltype(execs), decltype(lhs), decltype(rhs)>::value) {
                      execs(lhs, rhs);
                      return true;
                    }
                    return false;
                  }()));

    return found;
  }
};

template <typename T, typename E, std::size_t i, std::size_t size, typename... Executors> struct UnpackExecTuple {
  static bool execute(T &lhs, E &rhs, std::tuple<Executors...> execs) {
    if constexpr (i >= size) {
      return false;
    } else {

      auto exec = std::get<i>(execs);

      if constexpr (std::is_invocable<decltype(exec), decltype(lhs), decltype(rhs)>::value) {
        exec(lhs, rhs);
        return true;
      }

      return UnpackExecTuple<T, E, i + 1, size, Executors...>::execute(lhs, rhs, execs);
    }
  }
};

template <> struct Executor<std::tuple<void>> {
  template <typename T, typename E, typename... Executors>
  static bool execute(T &lhs, E &rhs, std::tuple<Executors...> execs) {
    return UnpackExecTuple<T, E, 0, std::tuple_size_v<decltype(execs)>, Executors...>::execute(lhs, rhs, execs);
  }
};
export {

  template <typename BaseLhs, typename BaseRhs, typename Holder, typename... Ts> struct StaticDispatcher;
  template <typename BaseLhs, typename BaseRhs, typename... Ts> struct StaticDispatcher<BaseLhs, BaseRhs, void, Ts...> {
    template <typename... Executors> static void dispatch(BaseLhs *lhs, BaseRhs *rhs, Executors... execs) {
      bool found = (... || ([&]() {
                      if (auto *l = dynamic_cast<Ts *>(lhs)) {
                        return (... || ([&]() {
                                  if (auto *r = dynamic_cast<Ts *>(rhs)) {
                                    return Executor<void>::execute(l, r, execs...);
                                  }
                                  return false;
                                }()));
                      }
                      return false;
                    }()));
      if (!found) {
        throw std::runtime_error{"Dispatcher was not found"};
      }
    }
  };

  template <typename BaseLhs, typename BaseRhs, typename... Ts>
  struct StaticDispatcher<BaseLhs, BaseRhs, std::tuple<void>, Ts...> {
    template <typename... Executors> static void dispatch(BaseLhs *lhs, BaseRhs *rhs, std::tuple<Executors...> execs) {
      bool found = (... || ([&]() {
                      if (auto *l = dynamic_cast<Ts *>(lhs)) {
                        return (... || ([&]() {
                                  if (auto *r = dynamic_cast<Ts *>(rhs)) {
                                    return Executor<std::tuple<void>>::execute(l, r, execs);
                                  }
                                  return false;
                                }()));
                      }
                      return false;
                    }()));
      if (!found) {
        throw std::runtime_error{"Tuple dispatcher was not found"};
      }
    }
  };

  template <typename BaseLhs, typename BaseRhs, typename... LhsTypes, typename... RhsTypes>
  auto make_static_dispatch(type_list<LhsTypes...>, type_list<RhsTypes...>) {
    return StaticDispatcher<BaseLhs, BaseRhs, LhsTypes..., RhsTypes...>();
  }
}

struct TypeIndexArrayHash {
  template <std::size_t size> std::size_t operator()(std::array<std::type_index, size> array) const {
    std::size_t hash{};
    if (size == 0)
      return hash;

    hash = array[0].hash_code();

    for (std::size_t i{}; i != size; ++i) {
      hash ^= array[i].hash_code() << 1;
    }
  
    return hash;
  }
};

export {
  template <typename... Args> struct DynamicDispatcher {
    static constexpr std::size_t n_args{sizeof...(Args)};
    using Callback = std::function<void(Args...)>;
    std::unordered_map<std::array<std::type_index, n_args>, Callback, TypeIndexArrayHash> callbacks_;

    template <typename... ActualArgs> void add(std::function<void(ActualArgs...)> callback) {
      callbacks_[std::array<std::type_index, n_args>{std::type_index(typeid(ActualArgs))...}] =
          [callback](Args... args) { callback(static_cast<ActualArgs>(args)...); };
    }

    void dispatch(Args... args) {
      auto it{callbacks_.find(std::array<std::type_index, n_args>{std::type_index(typeid(args))...})};

      if (it == callbacks_.end()) {
        throw std::runtime_error{"Cannot find dispatcher"};
      }
      return it->second(args...);
    }
  };
}