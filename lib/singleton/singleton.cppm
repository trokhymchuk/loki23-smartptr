module;

#include <atomic>
#include <cassert>
#include <mutex>

export module loki23.singleton;

export import loki23.singleton.creation;
export import loki23.singleton.lifetime;

export {

  template <typename T, CreationPolicy<T> CreationPol = CreateUsingNew<T>,
            LifetimePolicy<T> LifetimePol = DefaultLifetime<T>>
  class SingletonHolder {
  public:
    static T &instance() {
      T *p = pInstance_.load(std::memory_order_acquire);
      if (!p) {
        std::lock_guard lock{mutex_};
        p = pInstance_.load(std::memory_order_relaxed);
        if (!p) {
          if (destroyed_) {
            LifetimePol::on_dead_reference();
            destroyed_ = false;
          }
          p = CreationPol::create();
          pInstance_.store(p, std::memory_order_release);
          LifetimePol::schedule_destruction(p, &destroy_singleton);
        }
      }
      return *p;
    }

  private:
    static void destroy_singleton() {
      assert(!destroyed_);
      CreationPol::destroy(pInstance_.load(std::memory_order_acquire));
      pInstance_.store(nullptr, std::memory_order_release);
      destroyed_ = true;
    }

    SingletonHolder() = delete;

    static inline std::atomic<T *> pInstance_{nullptr};
    static inline bool destroyed_{false};
    static inline std::mutex mutex_;
  };

}
