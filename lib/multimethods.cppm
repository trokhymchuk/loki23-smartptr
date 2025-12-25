module;

#include <type_traits>
#include <stdexcept>

export module loki23:multimethods;

import :type_list;

template <typename T, typename E, typename... Executors>
bool execute(T &lhs, E &rhs, Executors... exec) {
  bool found = (... || ([&]() {
                  if constexpr (std::is_invocable<decltype(exec), decltype(lhs),
                                                  decltype(rhs)>::value) {
                    exec(lhs, rhs);
                    return true;
                  }
                  return false;
                }()));

  return found;
}

export {
  template <typename BaseLhs, typename BaseRhs, typename... Ts>
  struct StaticDispatcher {
    template <typename... Executors>
    static void Dispatch(BaseLhs *lhs, BaseRhs *rhs, Executors... exec) {
      bool found = (... || ([&]() {
                      if (auto *l = dynamic_cast<Ts *>(lhs)) {
                        return (... || ([&]() {
                                  if (auto *r = dynamic_cast<Ts *>(rhs)) {
                                    return execute(l, r, exec...);
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
}