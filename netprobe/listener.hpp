#pragma once

#include <iostream>
#include <string_view>

#include "packets.hpp"

struct PacketListener {
  virtual ~PacketListener() = default;

  virtual void on_packet    (const NetPacket&)                   {}
  virtual void on_packet_seq(const NetPacket&, const NetPacket&) {}
};

struct PacketLogger : PacketListener {
  static constexpr std::string_view type_name = "PacketLogger";

  void on_packet(const NetPacket& p) override {
    std::cout << p << "\n";
  }

  void on_packet_seq(const NetPacket& a, const NetPacket& b) override {
    std::cout << a << "\n" << b << "\n";
  }
};
