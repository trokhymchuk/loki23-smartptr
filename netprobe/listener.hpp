#pragma once

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <string_view>
#include <thread>

#include "packets.hpp"

struct PacketListener {
  PacketListener() : thread_{[this] { run(); }} {}

  virtual ~PacketListener() { stop(); }

  PacketListener(const PacketListener&) = delete;
  PacketListener& operator=(const PacketListener&) = delete;

  void push(SmartPtr<NetPacket, RefCountedMT> pkt) {
    {
      std::lock_guard lock{mutex_};
      queue_.push(std::move(pkt));
    }
    cv_.notify_one();
  }

  virtual void on_packet(const NetPacket&) {}

protected:
  void stop() {
    {
      std::lock_guard lock{mutex_};
      if (stop_) return;
      stop_ = true;
    }
    cv_.notify_one();
    if (thread_.joinable())
      thread_.join();
  }

  SmartPtr<NetPacket, RefCountedMT> last_packet_;

private:
  void run() {
    while (true) {
      SmartPtr<NetPacket, RefCountedMT> pkt;
      {
        std::unique_lock lock{mutex_};
        cv_.wait(lock, [this] { return !queue_.empty() || stop_; });
        if (queue_.empty()) return;
        pkt = std::move(queue_.front());
        queue_.pop();
      }
      on_packet(*pkt);
      last_packet_ = pkt;
    }
  }

  bool                                          stop_{false};
  std::mutex                                    mutex_;
  std::condition_variable                       cv_;
  std::queue<SmartPtr<NetPacket, RefCountedMT>> queue_;
  std::thread                                   thread_;
};

struct PacketLogger : PacketListener {
  static constexpr std::string_view type_name = "PacketLogger";

  ~PacketLogger() { stop(); }

  void on_packet(const NetPacket& p) override {
    std::cout << p << "\n";
  }
};
