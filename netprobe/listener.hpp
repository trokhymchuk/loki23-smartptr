#pragma once

#include <iostream>
#include "packets.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// Listener interface — override only the packet types you care about.
// The capturer resolves the concrete type before calling these.
// ─────────────────────────────────────────────────────────────────────────────
struct IPacketListener {
    virtual ~IPacketListener() = default;

    virtual void on_tcp (const TcpPacket&)  {}
    virtual void on_http(const HttpPacket&) {}
    virtual void on_dns (const DnsPacket&)  {}
};

// ─────────────────────────────────────────────────────────────────────────────
// PacketLogger — prints every packet with its resolved type prefix
// ─────────────────────────────────────────────────────────────────────────────
struct PacketLogger : IPacketListener {
    void on_tcp(const TcpPacket& p) override {
        std::cout << "[TCP]  " << p << "\n";
    }
    void on_http(const HttpPacket& p) override {
        std::cout << "[HTTP] " << p << "\n";
    }
    void on_dns(const DnsPacket& p) override {
        std::cout << "[DNS]  " << p << "\n";
    }
};
