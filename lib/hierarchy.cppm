module;

#include <functional>
#include <stdexcept>

export module loki23:hierarchy;

import :type_list;

export {

  template <typename T, typename E, typename... Args>
  concept Bindable = requires(T t) {
    { t.template bind<Args...>(std::declval<Args>()...) } -> std::same_as<void>;
  };

  template <typename T> struct Holder {
    Holder() {}
    virtual ~Holder() {
      if (initialized)
        value.~T();
    }

    bool initialized{};

    union {
      T value;
    };

    virtual T &get() {
      if (!initialized) {
        throw std::runtime_error{"Value is not initialized!"};
      }
      return value;
    }
    void *mem_location() { return static_cast<void *>(&value); }
    bool is_initialized() { return initialized; }
  };

  template <typename T>
  struct Dep {
    Holder<T> &holder;
  };

  template <typename T>
  Dep<T> dep(Holder<T> &h) { return {h}; }

  template <typename T>
  T &unwrap(Dep<T> d) { return d.holder.get(); }

  template <typename T>
  decltype(auto) unwrap(T &&v) { return std::forward<T>(v); }

  template <typename T> struct LazyHolder : Holder<T> {
    std::function<void(T *)> creator;

    template <typename... E> void bind(E... args) {
      creator = [=](T *where) { new (where) T{unwrap(args)...}; };
    }

    T &get() override {
      if (!Holder<T>::initialized) {
        creator(&(Holder<T>::value));
        Holder<T>::initialized = true;
      }
      return Holder<T>::value;
    }
  };

  template <typename T> struct ImmediateHolder : Holder<T> {
    std::function<void(T *)> creator;

    template <typename... E> void bind(E... args) {
      new (&(Holder<T>::value)) T{args...};
      Holder<T>::initialized = true;
    }
  };

  template <typename T> struct DefaultHolder : Holder<T> {
    T value{};

    T &get() { return value; }
    void *mem_location() { return static_cast<void *>(&value); }
  };
}

export {
  template <template <typename> typename Unit, typename T>
  struct GenScatterHierarchy;

  template <template <typename> class Unit, typename T>
  struct GenScatterHierarchy : public Unit<T> {};

  template <template <class> class Unit, typename... Ts>
  struct GenScatterHierarchy<Unit, type_list<Ts...>>
      : public GenScatterHierarchy<Unit, Ts>... {};

  template <typename T, template <class> class Unit, typename AnyList>
  Unit<T> &Field(GenScatterHierarchy<Unit, AnyList> &obj) {
    return static_cast<Unit<T> &>(obj);
  }
}

export {
  template <template <typename> typename Unit, typename T>
  struct GenLinearHierarchy;

  template <template <typename> class Unit, typename T>
  struct GenLinearHierarchy : public Unit<T> {};

  template <template <class> class Unit, typename T>
  struct GenLinearHierarchy<Unit, type_list<T>> : GenLinearHierarchy<Unit, T> {
  };

  template <template <class> class Unit, typename T, typename... Ts>
  struct GenLinearHierarchy<Unit, type_list<T, Ts...>>
      : public GenLinearHierarchy<Unit, T>,
        GenLinearHierarchy<Unit, type_list<Ts...>> {};

  template <typename T, template <class> class Unit, typename AnyList>
  Unit<T> &Field(GenLinearHierarchy<Unit, AnyList> &obj) {
    return static_cast<Unit<T> &>(obj);
  }

}