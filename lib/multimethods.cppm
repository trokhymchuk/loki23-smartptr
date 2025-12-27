module;

#include <array>
#include <functional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <unordered_map>

export module loki23:multimethods;

import :type_list;

template <typename Holder = void> struct Executor;
template <> struct Executor<void> {
  template <typename LhsT, typename RhsT, typename... Functors>
  static bool execute(LhsT &lhs, RhsT &rhs, Functors... execs) {
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

template <typename LhsT, typename RhsT, std::size_t i, std::size_t size, typename... Functors> struct UnpackExecTuple {
  static bool execute(LhsT &lhs, RhsT &rhs, std::tuple<Functors...> execs) {
    if constexpr (i >= size) {
      return false;
    } else {

      auto exec = std::get<i>(execs);

      if constexpr (std::is_invocable<decltype(exec), decltype(lhs), decltype(rhs)>::value) {
        exec(lhs, rhs);
        return true;
      }

      return UnpackExecTuple<LhsT, RhsT, i + 1, size, Functors...>::execute(lhs, rhs, execs);
    }
  }
};

template <> struct Executor<std::tuple<void>> {
  template <typename LhsT, typename RhsT, typename... Functors>
  static bool execute(LhsT &lhs, RhsT &rhs, std::tuple<Functors...> execs) {
    return UnpackExecTuple<LhsT, RhsT, 0, std::tuple_size_v<decltype(execs)>, Functors...>::execute(lhs, rhs, execs);
  }
};

template <typename BaseLhs, typename BaseRhs, typename ExecutorType, typename... Ts> struct StaticDispatcherExecutor {
  template <typename... Functors> static bool execute(BaseLhs *lhs, BaseRhs *rhs, Functors... execs) {
    return (... || ([&]() {
              if (auto *l = dynamic_cast<Ts *>(lhs)) {
                return (... || ([&]() {
                          if (auto *r = dynamic_cast<Ts *>(rhs)) {
                            return Executor<ExecutorType>::execute(l, r, execs...);
                          }
                          return false;
                        }()));
              }
              return false;
            }()));
  }
};

export {

  template <typename BaseLhs, typename BaseRhs, typename... Ts> struct StaticDispatcher;
  template <typename BaseLhs, typename BaseRhs, typename... Ts> struct StaticDispatcher<BaseLhs, BaseRhs, void, Ts...> {
    template <typename... Functors> static void dispatch(BaseLhs *lhs, BaseRhs *rhs, Functors... execs) {
      bool found = StaticDispatcherExecutor<BaseLhs, BaseRhs, void, Ts...>::execute(lhs, rhs, execs...);
      if (!found) {
        throw std::runtime_error{"Dispatcher was not found"};
      }
    }
  };

  template <typename BaseLhs, typename BaseRhs, typename... Ts>
  struct StaticDispatcher<BaseLhs, BaseRhs, std::tuple<void>, Ts...> {
    template <typename... Executors> static void dispatch(BaseLhs *lhs, BaseRhs *rhs, std::tuple<Executors...> execs) {
      bool found = StaticDispatcherExecutor<BaseLhs, BaseRhs, std::tuple<void>, Ts...>::execute(lhs, rhs, execs);
      if (!found) {
        throw std::runtime_error{"Dispatcher was not found"};
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
    static std::size_t hash{};

    if (size == 0)
      return hash;

    if (hash) {
      return hash;
    }

    hash = array[0].hash_code();

    for (std::size_t i{}; i != size; ++i) {
      hash ^= array[i].hash_code() << i;
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