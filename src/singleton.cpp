#include <iostream>

import loki23;

struct Config {
  int value = 42;
};

unsigned int get_longevity(Config *) { return 5; }

int main() {
  Config &c1 = SingletonHolder<Config>::instance();
  Config &c2 = SingletonHolder<Config>::instance();
  std::cout << "same instance: " << (&c1 == &c2) << '\n';
  std::cout << "value: " << c1.value << '\n';

  using StaticConfig = SingletonHolder<Config, CreateStatic<Config>, NoDestroy<Config>>;
  Config &cs = StaticConfig::instance();
  std::cout << "static value: " << cs.value << '\n';

  using MallocConfig = SingletonHolder<Config, CreateUsingMalloc<Config>>;
  Config &cm = MallocConfig::instance();
  std::cout << "malloc value: " << cm.value << '\n';
}
