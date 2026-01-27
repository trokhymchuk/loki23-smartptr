module;

#include <exception>
#include <iostream>

export module loki23.smart_ptr.checking;

export {

  template <typename Policy, typename Tptr>
  concept CheckingPolicy = requires(Policy policy, Tptr ptr) {
    { policy.on_init(ptr) } -> std::same_as<void>;
    { policy.on_dereference(ptr) } -> std::same_as<void>;
    { policy.on_release(ptr) } -> std::same_as<void>;
  };

  template <typename Tval, typename Tptr = Tval *> struct NoCheck {
    static void on_init(Tptr) {}
    static void on_dereference(Tptr) {}
    static void on_release(Tptr) {}
  };

  template <typename Tval, typename Tptr = Tval *> struct ExceptionCheck {
    static void on_init(Tptr ptr) {
      if (!ptr)
        throw std::invalid_argument("Null pointer initialization");
    }
    static void on_dereference(Tptr ptr) {
      if (!ptr)
        throw std::runtime_error("Null pointer dereference");
    }
    static void on_release(Tptr) {}
  };

  template <typename Tval, typename Tptr = Tval *> struct HaltCheck {
    static void on_init(Tptr ptr) {
      if (!ptr) {
        std::cerr << "Null pointer initialization\n";
        std::terminate();
      }
    }
    static void on_dereference(Tptr ptr) {
      if (!ptr) {
        std::cerr << "Null pointer dereference\n";
        std::terminate();
      }
    }
    static void on_release(Tptr) {}
  };
}