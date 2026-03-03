module;

#include <concepts>
#include <cstdlib>
#include <new>
#include <cstddef>

export module loki23.singleton.creation;

export {

  template <typename Policy, typename T>
  concept CreationPolicy = requires {
    { Policy::create() } -> std::same_as<T *>;
  } && requires(T *p) {
    { Policy::destroy(p) } -> std::same_as<void>;
  };

  template <typename T> struct CreateUsingNew {
    static T *create() { return new T; }
    static void destroy(T *p) { delete p; }
  };

  template <typename T> struct CreateUsingMalloc {
    static T *create() {
      void *p = std::malloc(sizeof(T));
      if (!p)
        return nullptr;
      return new (p) T;
    }
    static void destroy(T *p) {
      p->~T();
      std::free(p);
    }
  };

  template <typename T> struct CreateStatic {
    static T *create() {
      alignas(T) static std::byte storage[sizeof(T)];
      return new (&storage) T;
    }
    static void destroy(T *p) { p->~T(); }
  };

}
