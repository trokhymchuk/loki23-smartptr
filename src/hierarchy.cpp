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

#if 0

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

#if 0

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

template <>
struct AbstractFactory<A> {
    virtual A* DoCreate(int a) = 0;
    virtual ~AbstractFactory() = default;
};

using ABAbstractFactoryInterface = GenScatterHierarchy<AbstractFactory, type_list<A, B>>;

template <class T>
struct ConcereteFactory: AbstractFactory<T> {
        T* DoCreate() override {
        std::cout << "Creating " << typeid(T).name() << '\n';
        return new T();
    }
};

template <>
struct ConcereteFactory<A> : AbstractFactory<A> {
        A* DoCreate(int a) override {
        std::cout << "Creating " << typeid(A).name() << " with " << a << '\n';
        return new A();
    }
};

using ABFactory = GenLinearHierarchy<ConcereteFactory, type_list<A, B>>;

int main() {
  ABFactory factory;
  A *a = Field<A>(factory).DoCreate(6);
  B *b = Field<B>(factory).DoCreate();

  delete b;
  delete a;
}

#endif

#if 1

#include <iostream>

struct Logger {
  Logger() { std::cout << "Logger created\n"; }
  void log(const std::string &msg) { std::cout << "[LOG] " << msg << '\n'; }
};

struct DB {
  Logger &logger;
  int port;
  DB(Logger &logger, int port) : logger(logger), port(port) {
    std::cout << "DB created on port " << port << '\n';
    logger.log("DB connected");
  }
};

struct UserService {
  DB &db;
  Logger &logger;
  UserService(DB &db, Logger &logger) : db(db), logger(logger) {
    std::cout << "UserService created\n";
    logger.log("UserService ready");
  }
};

int main() {
  Container<Logger, DB, UserService> hierarchy;

  hierarchy.bind<Logger>();
  hierarchy.bind<DB>(From<Logger>{}, 5432);
  hierarchy.bind<UserService>(From<DB>{}, From<Logger>{});

  UserService &svc = hierarchy.get<UserService>();
  svc.logger.log("done");
}

#endif