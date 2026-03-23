module;

#include <array>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <unordered_map>

export module loki23:multimethods;

import :type_list;

export {
  enum class DispatchPolicy {
    FIRST_MATCH,
    ALL_MATCHES,
  };
}

export {
  struct NoOpNoMatchPolicy {
    template <typename... Args> static void no_match() {}
  };

  struct ExceptionNoMatchPolicy {
    template <typename... Args> static void no_match() { throw std::runtime_error{"Cannot find dispatcher"}; }
  };

  struct LogNoMatchPolicy {
    template <typename... Args> static void no_match() {
      std::cerr << "Cannot find dispatcher for: ";
      ((std::cerr << typeid(Args).name() << ", "), ...);
      std::cerr << '\n';
    }
  };
}


template <DispatchPolicy dispatch_policy, typename Holder = void> struct Executor;
template <> struct Executor<DispatchPolicy::FIRST_MATCH, void> {
  template <typename LhsT, typename RhsT, typename... Functors>
  static int execute(LhsT &lhs, RhsT &rhs, Functors... execs) {
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

template <> struct Executor<DispatchPolicy::ALL_MATCHES, void> {
  template <typename LhsT, typename RhsT, typename... Functors>
  static int execute(LhsT &lhs, RhsT &rhs, Functors... execs) {
    bool found = (... + ([&]() {
                    if constexpr (std::is_invocable<decltype(execs), decltype(lhs), decltype(rhs)>::value) {
                      execs(lhs, rhs);
                      return true;
                    }
                    return false;
                  }()));

    return found;
  }
};

template <DispatchPolicy dispatch_policy, typename LhsT, typename RhsT, std::size_t i, std::size_t size, typename... Functors> struct UnpackExecTuple {
  static int execute(LhsT &lhs, RhsT &rhs, std::tuple<Functors...> execs) {
    if constexpr (i >= size) {
      return false;
    } else {

      auto exec = std::get<i>(execs);

      int executed{};

      if constexpr (std::is_invocable<decltype(exec), decltype(lhs), decltype(rhs)>::value) {
        exec(lhs, rhs);
        if constexpr(dispatch_policy == DispatchPolicy::FIRST_MATCH) {
          return true;
        }
        executed = 1;
      }

      return executed + UnpackExecTuple<dispatch_policy, LhsT, RhsT, i + 1, size, Functors...>::execute(lhs, rhs, execs);
    }
  }
};

template <> struct Executor<DispatchPolicy::FIRST_MATCH, std::tuple<void>> {
  template <typename LhsT, typename RhsT, typename... Functors>
  static int execute(LhsT &lhs, RhsT &rhs, std::tuple<Functors...> execs) {
    return UnpackExecTuple<DispatchPolicy::FIRST_MATCH, LhsT, RhsT, 0, std::tuple_size_v<decltype(execs)>, Functors...>::execute(lhs, rhs, execs);
  }
};

template <> struct Executor<DispatchPolicy::ALL_MATCHES, std::tuple<void>> {
  template <typename LhsT, typename RhsT, typename... Functors>
  static int execute(LhsT &lhs, RhsT &rhs, std::tuple<Functors...> execs) {
    return UnpackExecTuple<DispatchPolicy::ALL_MATCHES, LhsT, RhsT, 0, std::tuple_size_v<decltype(execs)>, Functors...>::execute(lhs, rhs, execs);
  }
};


template <DispatchPolicy dispatch_policy, typename BaseLhs, typename BaseRhs, typename ExecutorType, typename... Ts>
struct StaticDispatcherExecutor;

template <typename BaseLhs, typename BaseRhs, typename ExecutorType, typename... Ts>
struct StaticDispatcherExecutor<DispatchPolicy::FIRST_MATCH, BaseLhs, BaseRhs, ExecutorType, Ts...> {
  template <typename... Functors> static int execute(BaseLhs *lhs, BaseRhs *rhs, Functors... execs) {
    return (... || ([&]() {
              if (auto *l = dynamic_cast<Ts *>(lhs)) {
                return (... || ([&]() {
                          if (auto *r = dynamic_cast<Ts *>(rhs)) {
                            Executor<DispatchPolicy::FIRST_MATCH, ExecutorType>::execute(l, r, execs...);
                            return 1;
                          }
                          return 0;
                        }()));
              }
              return false;
            }()));
  }
};

template <typename BaseLhs, typename BaseRhs, typename ExecutorType, typename... Ts>
struct StaticDispatcherExecutor<DispatchPolicy::ALL_MATCHES, BaseLhs, BaseRhs, ExecutorType, Ts...> {
  template <typename... Functors> static int execute(BaseLhs *lhs, BaseRhs *rhs, Functors... execs) {
    return (... + ([&]() {
              if (auto *l = dynamic_cast<Ts *>(lhs)) {
                return (... + ([&]() {
                          if (auto *r = dynamic_cast<Ts *>(rhs)) {
                            return Executor<DispatchPolicy::ALL_MATCHES, ExecutorType>::execute(l, r, execs...);
                          }
                          return 0;
                        }()));
              }
              return 0;
            }()));
  }
};

export {

  template <DispatchPolicy dispatch_policy, typename NoMatchPolicy, typename BaseLhs, typename BaseRhs, typename... Ts>
  struct StaticDispatcher;
  template <DispatchPolicy dispatch_policy, typename NoMatchPolicy, typename BaseLhs, typename BaseRhs, typename... Ts>
  struct StaticDispatcher<dispatch_policy, NoMatchPolicy, BaseLhs, BaseRhs, void, Ts...> {
    template <typename... Functors> static void dispatch(BaseLhs *lhs, BaseRhs *rhs, Functors... execs) {
      bool found =
          StaticDispatcherExecutor<dispatch_policy, BaseLhs, BaseRhs, void, Ts...>::execute(lhs, rhs, execs...);
      if (!found) {
        NoMatchPolicy::template no_match<BaseLhs *, BaseRhs *>();
        return;
      }
    }
  };

  template <DispatchPolicy dispatch_policy, typename NoMatchPolicy, typename BaseLhs, typename BaseRhs, typename... Ts>
  struct StaticDispatcher<dispatch_policy, NoMatchPolicy, BaseLhs, BaseRhs, std::tuple<void>, Ts...> {
    template <typename... Executors> static void dispatch(BaseLhs *lhs, BaseRhs *rhs, std::tuple<Executors...> execs) {
      bool found = StaticDispatcherExecutor<dispatch_policy, BaseLhs, BaseRhs, std::tuple<void>, Ts...>::execute(
          lhs, rhs, execs);
      if (!found) {
        NoMatchPolicy::template no_match<BaseLhs *, BaseRhs *>();
        return;
      }
    }
  };
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
  template <DispatchPolicy dispatch_policy, typename NoMatchPolicy, typename... Args> struct DynamicDispatcher {
    static constexpr std::size_t n_args{sizeof...(Args)};
    using Callback = std::function<void(Args...)>;

    std::conditional_t<dispatch_policy == DispatchPolicy::FIRST_MATCH,
                       std::unordered_map<std::array<std::type_index, n_args>, Callback, TypeIndexArrayHash>,
                       std::unordered_multimap<std::array<std::type_index, n_args>, Callback, TypeIndexArrayHash>>
        callbacks_;

    template <typename... ActualArgs> void add(std::function<void(ActualArgs...)> callback) {
      callbacks_.insert({std::array<std::type_index, n_args>{std::type_index(typeid(ActualArgs))...},
                         [callback](Args... args) { callback(static_cast<ActualArgs>(args)...); }});
    }

    void dispatch(Args... args) {
      auto [begin, end]{callbacks_.equal_range(std::array<std::type_index, n_args>{std::type_index(typeid(args))...})};

      if (begin == callbacks_.end() && end == callbacks_.end()) {
        NoMatchPolicy::template no_match<Args...>();
        return;
      }

      for (auto it{begin}; it != end; ++it) {
        it->second(args...);
      }
    }
  };
}