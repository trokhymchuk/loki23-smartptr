#include <concepts>
#include <initializer_list>
#include <iostream>
#include <string>

import loki23;

#if 0

struct DB {
  DB(const std::string &username, const std::string &password) {
    std::cout << "Connected to DB using " << username << " and " << password
              << '\n';
  }
};

template <template <typename> typename H, typename T>
  requires Bindable<H<T>, T> && std::derived_from<H<T>, Holder<T>>
struct DBUser {
  H<T> &holder;

  void use() {
    holder.get();
    std::cout << "I am using database!" << '\n';
  }
};

template <typename T, typename E>
  requires Bindable<T, E>
void bind(T &holder, E value) {
  holder.bind(value);
}

int main() {
  GenScatterHierarchy<
        // ImmediateHolder,
      LazyHolder,
      type_list<std::string, int, DB, type_list<std::vector<double>>>>
      obj;
  // Field<std::string>(obj).bind("abc");
  // Field<int>(obj).bind(5);
  Field<DB>(obj).bind("artem", "password");
  Field<std::vector<double>>(obj).bind(std::vector<double>{1, 2, 3});
  //   bind(Field<int>(obj), 5);
  // std::cout << Field<std::string>(obj).get() << "\n";
  //   std::cout << Field<int>(obj).get() << "\n";
  std::cout << Field<std::vector<double>>(obj).is_initialized() << '\n';
  std::cout << Field<std::vector<double>>(obj).get().size() << "\n";
  std::cout << Field<std::vector<double>>(obj).is_initialized() << '\n';

  DBUser p1{Field<DB>(obj)};
  p1.use();
}
#endif

#if 0
int main() {
  GenLinearHierarchy<
      LazyHolder, type_list<std::string, int, type_list<std::vector<double>>>>
      obj;
  Field<std::string>(obj).bind("abc");
  Field<int>(obj).bind(5);
  Field<std::vector<double>>(obj).bind(std::vector<double>{1, 2, 3});
  std::cout << Field<std::string>(obj).get() << "\n";
  std::cout << Field<int>(obj).get() << "\n";
  std::cout << Field<std::vector<double>>(obj).mem_location() << '\n';
  std::cout << Field<std::vector<double>>(obj).get().size() << "\n";
  std::cout << Field<std::vector<double>>(obj).mem_location() << '\n';
}
#endif

#if 1

struct Base {
  virtual ~Base() = default;
};
struct A : Base {};
struct B : Base {};

template <typename T>
struct AbstractFactory {
    virtual T* DoCreate() = 0;
    virtual ~AbstractFactory() = default;
};

using ABAbstractFactoryInterface = GenScatterHierarchy<AbstractFactory, type_list<A, B>>;

template <class T, class Base>
struct AbstractConcereteFactory : public Base {
        T* DoCreate() override {
        std::cout << "Creating " << typeid(T).name() << '\n';
        return new T();
    }
};

template <typename T>
using ConcreteFactory = AbstractConcereteFactory<T, AbstractFactory<T>>;

using ABFactory = GenLinearHierarchy<ConcreteFactory, type_list<A, B>>;

int main() {
  ABFactory factory;
  A *a = Field<A>(factory).DoCreate();
  B *b = Field<B>(factory).DoCreate();

  delete b;
  delete a;
}

#endif