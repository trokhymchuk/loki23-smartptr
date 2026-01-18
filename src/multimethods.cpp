#include <complex>
#include <cstdlib>
#include <format>
#include <functional>
#include <iostream>
#include <regex>
#include <string>

import loki23;

#if 0

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

#endif

#if 1

struct Number {
  const std::string representation;

  Number(std::string repr) : representation{std::move(repr)} {}

  virtual ~Number() = default;
};

std::ostream &operator<<(std::ostream &os, const Number &rhs) { return os << rhs.representation; }

struct IntNumber : public Number {
  const int number;
  IntNumber(int num) : Number(std::to_string(num)), number{num} {}
  IntNumber(std::string num) : Number(std::move(num)), number{std::stoi(representation)} {}
};

IntNumber operator+(const IntNumber &lhs, const IntNumber &rhs) { return IntNumber{lhs.number + rhs.number}; }
IntNumber operator+(const IntNumber &lhs, const Number &rhs) { return lhs + IntNumber{rhs.representation}; }

struct DoubleNumber : public Number {
  const double number;
  DoubleNumber(double num) : Number(std::to_string(num)), number{num} {}
  DoubleNumber(std::string num) : Number(std::move(num)), number{std::stod(representation)} {}
};

DoubleNumber operator+(const DoubleNumber &lhs, const DoubleNumber &rhs) {
  return DoubleNumber{lhs.number + rhs.number};
}
DoubleNumber operator+(const DoubleNumber &lhs, const Number &rhs) { return lhs + DoubleNumber{rhs.representation}; }

static std::complex<double> parse_complex(const std::string &representation) {
  std::regex pattern(R"(^([+-]?\d*\.?\d+)\s*([+-])\s*(\d*\.?\d+)[ij]$)");
  std::smatch matches;

  if (std::regex_match(representation, matches, pattern)) {
    double real = std::stod(matches[1].str());
    double imag = std::stod(matches[3].str());
    if (matches[2].str() == "-") {
      imag = -imag;
    }

    return std::complex<double>(real, imag);
  } else {
    throw std::runtime_error("Invalid complex number format");
  }
}
struct ComplexNumber : public Number {
  const std::complex<double> number;
  ComplexNumber(std::complex<double> num) : Number(std::format("{} + {}i", num.real(), num.imag())), number{num} {}
  ComplexNumber(std::string num) : Number(std::move(num)), number{parse_complex(representation)} {}
};
ComplexNumber operator+(const ComplexNumber &lhs, const ComplexNumber &rhs) {
  return ComplexNumber{lhs.number + rhs.number};
}
ComplexNumber operator+(const ComplexNumber &lhs, const Number &rhs) { return lhs + ComplexNumber{rhs.representation}; }

// void f(IntNumber *iNumber, DoubleNumber *dNumb) {}
// void f(DoubleNumber *dNumber, IntNumber *iNumber) {}

#if 1
int main() {
  Number *pInt = new IntNumber{"1"};
  Number *pDouble = new DoubleNumber{"2.2"};
  Number *pComplex = new ComplexNumber{"3 + 3i"};

  auto execs = std::make_tuple(
      [](IntNumber *iNumber, DoubleNumber *dNumber) {
        std::cout << "Int first, double second: ";
      },
      [](DoubleNumber *dNumber, IntNumber *iNumber) {
        std::cout << "Double first, int second: ";
      },
      [](IntNumber *iNumber, DoubleNumber *dNumber) {
        std::cout << (*iNumber + *dNumber) << '\n';
      },
      [](DoubleNumber *dNumber, IntNumber *iNumber) {
        std::cout << (*dNumber + *iNumber) << '\n';
      });

  using dispatcher = StaticDispatcher<DispatchPolicy::ALL_MATCHES, LogNoMatchPolicy, Number, Number, std::tuple<void>,
                                      IntNumber, DoubleNumber, ComplexNumber>;

  dispatcher::dispatch(pInt, pDouble, execs);

  // f(pInt, pDouble);

  dispatcher::dispatch(pDouble, pInt, execs);
  dispatcher::dispatch(pDouble, pComplex, execs);

  delete pComplex;
  delete pDouble;
  delete pInt;
}
#endif

#if 0
int main() {
  IntNumber int_number = IntNumber{"1"};
  DoubleNumber double_number = DoubleNumber{"2.2"};
  ComplexNumber complex_number = ComplexNumber{"3 + 3i"};

  Number &rInt = int_number;
  Number &rDouble = double_number;
  Number &rComplex = complex_number;

  auto dispatcher = DynamicDispatcher<DispatchPolicy::ALL_MATCHES, LogNoMatchPolicy, Number &, Number &>{};

  std::function l1 = [](IntNumber &iNumber, DoubleNumber &dNumber) {
    std::cout << "Int first, double second ";
  };
  std::function l2 = [](DoubleNumber &dNumber, IntNumber &iNumber) {
    std::cout << "Double first, int second ";
  };
  std::function l3 = [](IntNumber &iNumber, DoubleNumber &dNumber) {
    std::cout << (iNumber + dNumber) << '\n';
  };
  std::function l4 = [](DoubleNumber &dNumber, IntNumber &iNumber) {
    std::cout << (dNumber + iNumber) << '\n';
  };
  dispatcher.add(l3);
  dispatcher.add(l4);

  dispatcher.add(l1);
  dispatcher.add(l2);

  dispatcher.dispatch(rInt, rDouble);
  dispatcher.dispatch(rDouble, rInt);

  dispatcher.dispatch(rDouble, rComplex);
}

#endif

#endif