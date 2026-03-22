module;

#include <cassert>
#include <iostream>
#include <stdexcept>

export module loki23.smart_ptr.checking;

export {

  template <typename Policy>
  concept CheckingPolicy = requires(bool b) {
    { Policy::check(b) } -> std::same_as<void>;
  };

  struct NoCheck {
    static void check(bool) noexcept {}
  };

  struct AssertCheck {
    static void check(bool valid) noexcept {
      assert(valid && "SmartPtr: invalid handle");
    }
  };

  struct ExceptionCheck {
    static void check(bool valid) {
      if (!valid) throw std::runtime_error("SmartPtr: invalid handle");
    }
  };

  struct HaltCheck {
    static void check(bool valid) noexcept {
      if (!valid) {
        std::cerr << "SmartPtr: invalid handle\n";
        std::terminate();
      }
    }
  };
}
