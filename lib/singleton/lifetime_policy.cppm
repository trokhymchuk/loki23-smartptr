module;

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <functional>
#include <stdexcept>
#include <vector>

export module loki23.singleton.lifetime;

export {

  template <typename Policy, typename T>
  concept LifetimePolicy = requires(T *p, void (*fn)()) {
    { Policy::schedule_destruction(p, fn) } -> std::same_as<void>;
    { Policy::on_dead_reference() } -> std::same_as<void>;
  };

  class LifetimeTracker {
  public:
    explicit LifetimeTracker(unsigned int x) : longevity_{x} {}
    virtual ~LifetimeTracker() = default;

    static bool compare(const LifetimeTracker *lhs, const LifetimeTracker *rhs) {
      return lhs->longevity_ < rhs->longevity_;
    }

  private:
    unsigned int longevity_;
  };

}

std::vector<LifetimeTracker *> tracker_array;

void at_exit_fn() {
  assert(!tracker_array.empty());
  LifetimeTracker *top = tracker_array.back();
  tracker_array.pop_back();
  delete top;
}

export {

  template <typename T, typename Destroyer> class ConcreteLifetimeTracker : public LifetimeTracker {
  public:
    ConcreteLifetimeTracker(T *p, unsigned int longevity, Destroyer d)
        : LifetimeTracker{longevity}, tracked_{p}, destroyer_{d} {}

    ~ConcreteLifetimeTracker() override { destroyer_(tracked_); }

  private:
    T *tracked_;
    Destroyer destroyer_;
  };

  template <typename T, typename Destroyer>
  void set_longevity(T *obj, unsigned int longevity, Destroyer d) {
    auto *tracker = new ConcreteLifetimeTracker<T, Destroyer>{obj, longevity, d};

    auto pos = std::upper_bound(tracker_array.begin(), tracker_array.end(), tracker,
                                LifetimeTracker::compare);
    tracker_array.insert(pos, tracker);

    std::atexit(at_exit_fn);
  }

  template <typename T> struct DefaultLifetime {
    static void schedule_destruction(T *, void (*fn)()) { std::atexit(fn); }
    [[noreturn]] static void on_dead_reference() {
      throw std::logic_error{"Dead Reference Detected"};
    }
  };

  template <typename T> class PhoenixSingleton {
  public:
    static void schedule_destruction(T *, void (*fn)()) {
      if (!destroyed_once_)
        std::atexit(fn);
    }
    static void on_dead_reference() { destroyed_once_ = true; }

  private:
    static inline bool destroyed_once_ = false;
  };

  template <typename T> struct SingletonWithLongevity {
    static void schedule_destruction(T *p, void (*fn)()) {
      set_longevity(p, get_longevity(p), [fn] { fn(); });
    }
    [[noreturn]] static void on_dead_reference() {
      throw std::logic_error{"Dead Reference Detected"};
    }
  };

  template <typename T> struct NoDestroy {
    static void schedule_destruction(T *, void (*) ()) {}
    static void on_dead_reference() {}
  };

}
