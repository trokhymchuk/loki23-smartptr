#include <chrono>
#include <iostream>
#include <thread>
#include <IPv4Layer.h>
#include <Packet.h>

#include "packets.hpp"
#include "listener.hpp"
#include "capturer.hpp"

import loki23;

using Capturer = SingletonHolder<PacketCapturer>;

int main(int argc, char* argv[]) {
    // ── Register listeners before opening the interface ──────────────────────
    PacketLogger logger;
    Capturer::instance().add_listener(&logger);

    // ── Open interface and start live capture ────────────────────────────────
    // Pass an interface name as the first argument, or omit for the default.
    const std::string iface = (argc > 1) ? argv[1] : "";
    Capturer::instance().open(iface);
    std::cout << "Capturing on " << Capturer::instance().interface_name()
              << " for 5 seconds...\n";

    std::this_thread::sleep_for(std::chrono::seconds{50});

    Capturer::instance().close();
    std::cout << "Capture done.\n\n";

    // ── Manual dispatch — show the listener works without live capture too ───
    std::cout << "--- synthetic packets ---\n";

    // ── TcpPacket ────────────────────────────────────────────────────────────
    TcpPacket tcp;
    tcp.src_ip      = "192.168.1.10";
    tcp.dst_ip      = "93.184.216.34";
    tcp.size        = 60;
    tcp.timestamp   = std::chrono::system_clock::now();
    tcp.src_port    = 54321;
    tcp.dst_port    = 443;
    tcp.seq_num     = 1000;
    tcp.ack_num     = 0;
    tcp.syn         = true;
    tcp.window_size = 65535;
    Capturer::instance().dispatch(tcp);

    // ── HttpPacket ───────────────────────────────────────────────────────────
    HttpPacket http;
    http.src_ip       = "192.168.1.10";
    http.dst_ip       = "93.184.216.34";
    http.size         = 380;
    http.timestamp    = std::chrono::system_clock::now();
    http.src_port     = 54322;
    http.dst_port     = 80;
    http.seq_num      = 2000;
    http.ack          = true;
    http.psh          = true;
    http.method       = "GET";
    http.url          = "/index.html";
    http.http_version = "HTTP/1.1";
    http.host         = "example.com";
    http.headers["Accept"] = "text/html";
    Capturer::instance().dispatch(http);

    // ── DnsPacket ────────────────────────────────────────────────────────────
    DnsPacket dns;
    dns.src_ip      = "192.168.1.10";
    dns.dst_ip      = "8.8.8.8";
    dns.size        = 64;
    dns.timestamp   = std::chrono::system_clock::now();
    dns.query_id    = 0xABCD;
    dns.domain      = "example.com";
    dns.record_type = "A";
    dns.is_response = true;
    dns.answers     = {"93.184.216.34"};
    Capturer::instance().dispatch(dns);

    Capturer::instance().remove_listener(&logger);
}
