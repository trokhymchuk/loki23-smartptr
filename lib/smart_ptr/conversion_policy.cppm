module;

#include <exception>
#include <iostream>

export module loki23.smart_ptr.conversion;

export {
  template <typename Policy, typename Tptr>
  concept ConversionPolicy = requires {
    { Policy::implicit_convert() } -> std::same_as<bool>;
  };

  struct AllowConversion {
    static constexpr bool implicit_convert() { return true; }
  };

  struct DisallowConversion {
    static constexpr bool implicit_convert() { return false; }
  };
}