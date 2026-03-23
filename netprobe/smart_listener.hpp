#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "listener.hpp"
#include "packets.hpp"

// Tracks DNS resolutions and correlates subsequent TCP/HTTP connections to the
// resolved IPs to infer which hosts the user is communicating with.
struct SmartListener : PacketListener {
  static constexpr std::string_view type_name = "SmartListener";

  void on_packet(const NetPacket& p) override {
    if (auto* d = dynamic_cast<const DnsPacket*>(&p)) {
      if (d->is_response && !d->domain.empty())
        for (const auto& ip : d->answers)
          ip_to_domain_[ip] = d->domain;
      return;
    }

    if (auto* h = dynamic_cast<const HttpPacket*>(&p)) {
      if (!h->is_response)
        note_connection("HTTP", h->dst_ip, h->dst_port, h->host.empty() ? "" : h->host);
      return;
    }

    if (auto* t = dynamic_cast<const TcpPacket*>(&p)) {
      if (t->syn && !t->ack)
        note_connection("TCP", t->dst_ip, t->dst_port, "");
    }
  }

  // Detects DNS response immediately followed by a connection to one of its answers.
  void on_packet_seq(const NetPacket& a, const NetPacket& b) override {
    auto* dns = dynamic_cast<const DnsPacket*>(&a);
    if (!dns || !dns->is_response || dns->answers.empty()) return;

    std::string dst_ip;
    uint16_t    dst_port = 0;
    if (auto* h = dynamic_cast<const HttpPacket*>(&b)) {
      dst_ip = h->dst_ip; dst_port = h->dst_port;
    } else if (auto* t = dynamic_cast<const TcpPacket*>(&b)) {
      dst_ip = t->dst_ip; dst_port = t->dst_port;
    }
    if (dst_ip.empty()) return;

    for (const auto& ip : dns->answers) {
      if (ip == dst_ip) {
        std::cout << "[SMART] DNS for " << dns->domain
                  << " immediately followed by connection to "
                  << dst_ip << ":" << dst_port << "\n";
        return;
      }
    }
  }

private:
  void note_connection(const char* proto, const std::string& dst_ip,
                       uint16_t dst_port, const std::string& host) {
    auto it = ip_to_domain_.find(dst_ip);
    const std::string& domain = (it != ip_to_domain_.end()) ? it->second : host;
    if (domain.empty()) return;

    auto key = dst_ip + ":" + std::to_string(dst_port);
    if (reported_.count(key)) return;
    reported_.insert(key);

    std::cout << "[SMART] User likely visited " << domain
              << " (" << proto << " -> " << dst_ip << ":" << dst_port << ")\n";
  }

  std::unordered_map<std::string, std::string> ip_to_domain_;
  std::unordered_set<std::string>              reported_;
};
