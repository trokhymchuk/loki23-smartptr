#pragma once

#include <cstdint>
#include <ostream>
#include <string_view>
#include <unordered_map>

#include "listener.hpp"
#include "packets.hpp"

struct StatisticsListener : PacketListener {
  static constexpr std::string_view type_name = "StatisticsListener";

  ~StatisticsListener() { stop(); }

  struct TcpStats {
    uint64_t packets = 0;
    uint64_t bytes   = 0;
    uint64_t syn     = 0;
    uint64_t fin     = 0;
    uint64_t rst     = 0;
  };

  struct HttpStats {
    uint64_t requests  = 0;
    uint64_t responses = 0;
    uint64_t bytes     = 0;
    std::unordered_map<std::string, uint64_t> methods;
    std::unordered_map<int,         uint64_t> status_codes;
  };

  struct DnsStats {
    uint64_t queries   = 0;
    uint64_t responses = 0;
    uint64_t bytes     = 0;
    std::unordered_map<std::string, uint64_t> record_types;
    std::unordered_map<std::string, uint64_t> domains;
  };

  struct UdpStats {
    uint64_t packets = 0;
    uint64_t bytes   = 0;
  };

  TcpStats  tcp{};
  HttpStats http{};
  DnsStats  dns{};
  UdpStats  udp{};

  void on_packet(const NetPacket& p) override {
    if (auto* h = dynamic_cast<const HttpPacket*>(&p)) {
      http.bytes += p.size;
      if (h->is_response) {
        ++http.responses;
        ++http.status_codes[h->status_code];
      } else {
        ++http.requests;
        ++http.methods[h->method];
      }
    } else if (auto* t = dynamic_cast<const TcpPacket*>(&p)) {
      ++tcp.packets;
      tcp.bytes += p.size;
      if (t->syn) ++tcp.syn;
      if (t->fin) ++tcp.fin;
      if (t->rst) ++tcp.rst;
    } else if (auto* u = dynamic_cast<const UdpPacket*>(&p)) {
      ++udp.packets;
      udp.bytes += p.size;
    } else if (auto* d = dynamic_cast<const DnsPacket*>(&p)) {
      dns.bytes += p.size;
      if (d->is_response) ++dns.responses;
      else                ++dns.queries;
      if (!d->record_type.empty()) ++dns.record_types[d->record_type];
      if (!d->domain.empty())      ++dns.domains[d->domain];
    }
  }

  void print_report(std::ostream& os) const {
    os << "=== packet statistics ===\n";

    os << "TCP   packets=" << tcp.packets << "  bytes=" << tcp.bytes
       << "  SYN=" << tcp.syn << "  FIN=" << tcp.fin << "  RST=" << tcp.rst << "\n";

    os << "HTTP  requests=" << http.requests << "  responses=" << http.responses
       << "  bytes=" << http.bytes << "\n";
    for (auto& [m, n] : http.methods)
      os << "        method " << m << ": " << n << "\n";
    for (auto& [c, n] : http.status_codes)
      os << "        status " << c << ": " << n << "\n";

    os << "UDP   packets=" << udp.packets << "  bytes=" << udp.bytes << "\n";

    os << "DNS   queries=" << dns.queries << "  responses=" << dns.responses
       << "  bytes=" << dns.bytes << "\n";
    for (auto& [t, n] : dns.record_types)
      os << "        type " << t << ": " << n << "\n";
    for (auto& [d, n] : dns.domains)
      os << "        domain " << d << ": " << n << "\n";
  }
};
