#include <iostream>

import loki23;

struct Base {
  virtual ~Base() = default;
};
struct A : Base {};
struct B : Base {};

int main() {
  Base *pA = new A;
  Base *pB = new B;

  StaticDispatcher<Base, Base, A, B>::Dispatch(
      pA, pB,
      [](int *l, auto *r) {
        std::cout << "Found types: " << typeid(*l).name() << " and "
                  << typeid(*r).name() << "\n";
      },
      [](A *a, B *b) { std::cout << "a: " << a << ", b: " << b << '\n'; });

  delete pA;
  delete pB;
}