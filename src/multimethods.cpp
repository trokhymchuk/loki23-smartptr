#include <functional>
#include <iostream>

import loki23;

struct Base {
  virtual ~Base() = default;
};
struct A : Base {};
struct B : Base {};

#if 0
int main() {
  Base *pA = new A;
  Base *pB = new B;

  StaticDispatcher<Base, Base, std::tuple<void>, A, B>::dispatch(
      pA, pB,
      std::make_tuple(
          [](int *l, auto *r) {
            std::cout << "Found types: " << typeid(*l).name() << " and " << typeid(*r).name() << "\n";
          },
          [](A *a, B *b) { std::cout << "a: " << a << ", b: " << b << '\n'; }));

  delete pA;
  delete pB;
}
#endif

#if 1
int main() {
  A a;
  B b;

  Base &rA = a;
  Base &rB = b;

  auto dispatcher = DynamicDispatcher<Base &, Base &>{};

  std::function<void(A &, A &)> l1 = [](A &a1, A &a2) { std::cout << "a1: " << &a1 << ", a2: " << &a2 << '\n'; };
  std::function<void(A &, B &)> l2 = [](A &a, B &b) { std::cout << "a: " << &a << ", b: " << &b << '\n'; };

  dispatcher.add(l1);
  dispatcher.add(l2);

  dispatcher.dispatch(rA, rB);
}

#endif