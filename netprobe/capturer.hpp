#pragma once

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include <Packet.h>

#include "listener.hpp"
#include "packets.hpp"

class PacketCapturer {
public:
  virtual ~PacketCapturer() = default;

  PacketCapturer() = default;
  PacketCapturer(const PacketCapturer &) = delete;
  PacketCapturer &operator=(const PacketCapturer &) = delete;

  virtual void open(std::string arg = {}) = 0;
  virtual void close() = 0;

  void add_listener(PacketListener *listener) {
    if (!listener)
      throw std::invalid_argument{"null listener"};
    listeners_.push_back(listener);
  }

  void remove_listener(PacketListener *listener) {
    auto it = std::find(listeners_.begin(), listeners_.end(), listener);
    if (it != listeners_.end())
      listeners_.erase(it);
  }

  void dispatch(const NetPacket &pkt) {
    for (auto *l : listeners_)
      l->on_packet(pkt);
    if (last_packet_.valid())
      for (auto *l : listeners_)
        l->on_packet_seq(*last_packet_, pkt);
    last_packet_ = SmartPtr<NetPacket, NoCopy>{pkt.deep_copy()};
  }

protected:
  static NetPacket *parse(pcpp::RawPacket *raw) {
    pcpp::Packet parsed{raw};
    try { return new DnsPacket{parsed, raw};  } catch (const std::invalid_argument &) {}
    try { return new HttpPacket{parsed, raw}; } catch (const std::invalid_argument &) {}
    try { return new TcpPacket{parsed, raw};  } catch (const std::invalid_argument &) {}
    return nullptr;
  }

  std::vector<PacketListener *>    listeners_;
  SmartPtr<NetPacket, NoCopy>      last_packet_;
};
