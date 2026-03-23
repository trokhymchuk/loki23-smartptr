#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include "listener.hpp"
#include "packets.hpp"

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

  void on_packet_seq(const NetPacket& a, const NetPacket& b) override {
    using Dispatcher = StaticDispatcher<DispatchPolicy::FIRST_MATCH, NoOpNoMatchPolicy,
                                        NetPacket, NetPacket, std::tuple<void>,
                                        DnsPacket, HttpPacket, TcpPacket, UdpPacket>;
    Dispatcher::dispatch(
        const_cast<NetPacket*>(&a),
        const_cast<NetPacket*>(&b),
        std::make_tuple(
            [this](DnsPacket* dns, HttpPacket* h) { check_dns_connection(dns, h->dst_ip, h->dst_port); },
            [this](DnsPacket* dns, TcpPacket*  t) { check_dns_connection(dns, t->dst_ip,  t->dst_port); },
            [this](DnsPacket* dns, UdpPacket*  u) { check_dns_connection(dns, u->dst_ip,  u->dst_port); }
        )
    );
  }

private:
  void check_dns_connection(DnsPacket* dns, const std::string& dst_ip, uint16_t dst_port) {
    if (!dns->is_response || dns->answers.empty()) return;
    for (const auto& ip : dns->answers) {
      if (ip == dst_ip) {
        std::cout << "[SMART] DNS for " << dns->domain
                  << " immediately followed by connection to "
                  << dst_ip << ":" << dst_port << "\n";
        return;
      }
    }
  }

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
