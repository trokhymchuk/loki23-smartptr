#pragma once

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <string>
#include <vector>

#include <arpa/inet.h>

#include <DnsLayer.h>
#include <HttpLayer.h>
#include <IPv4Layer.h>
#include <Packet.h>
#include <PcapLiveDevice.h>
#include <PcapLiveDeviceList.h>
#include <TcpLayer.h>
#include <UdpLayer.h>

#include "listener.hpp"
#include "packets.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// PacketCapturer — opens a live network interface, parses incoming packets, and
// fans them out to registered IPacketListener instances.
//
// Intended to be used via SingletonHolder<PacketCapturer>.
// Typical lifecycle:
//   auto& cap = SingletonHolder<PacketCapturer>::instance();
//   cap.add_listener(&my_listener);
//   cap.open();          // starts capture on the default (first) interface
//   ...
//   cap.close();         // stops capture; also called by the destructor
// ─────────────────────────────────────────────────────────────────────────────
class PacketCapturer {
public:
    PacketCapturer() = default;   // default-constructible for SingletonHolder

    ~PacketCapturer() { close(); }

    PacketCapturer(const PacketCapturer&)            = delete;
    PacketCapturer& operator=(const PacketCapturer&) = delete;

    // ── Interface control ───────────────────────────────────────────────────

    // Open `interface_name` (or the first available device if empty) and start
    // live packet capture.  Captured packets are parsed and dispatched to every
    // registered listener from the capture thread.
    void open(const std::string& interface_name = {}) {
        if (device_) throw std::runtime_error{"already open"};

        if (interface_name.empty()) {
            auto& devices = pcpp::PcapLiveDeviceList::getInstance()
                                .getPcapLiveDevicesList();
            if (devices.empty())
                throw std::runtime_error{"no network interfaces found"};
            device_ = devices.front();
        } else {
            device_ = pcpp::PcapLiveDeviceList::getInstance()
                          .getDeviceByName(interface_name);
            if (!device_)
                throw std::runtime_error{"interface not found: " + interface_name};
        }

        if (!device_->open())
            throw std::runtime_error{"cannot open: " + std::string{device_->getName()}};

        device_->startCapture(on_packet_arrives, this);
    }

    void close() {
        if (!device_) return;
        device_->stopCapture();
        device_->close();
        device_ = nullptr;
    }

    const std::string interface_name() const {
        return device_ ? device_->getName() : "";
    }

    // ── Listener registration ───────────────────────────────────────────────

    void add_listener(IPacketListener* listener) {
        if (!listener) throw std::invalid_argument{"null listener"};
        listeners_.push_back(listener);
    }

    void remove_listener(IPacketListener* listener) {
        auto it = std::find(listeners_.begin(), listeners_.end(), listener);
        if (it != listeners_.end())
            listeners_.erase(it);
    }

    // ── Manual dispatch (also used by the capture callback) ────────────────
    // Resolves the concrete packet type and routes to the matching listener
    // method.  HttpPacket is checked before TcpPacket because it inherits
    // from it — dynamic_cast to TcpPacket would succeed for both otherwise.

    void dispatch(const NetPacket& pkt) const {
        if (auto* p = dynamic_cast<const HttpPacket*>(&pkt)) {
            for (IPacketListener* l : listeners_) l->on_http(*p);
        } else if (auto* p = dynamic_cast<const TcpPacket*>(&pkt)) {
            for (IPacketListener* l : listeners_) l->on_tcp(*p);
        } else if (auto* p = dynamic_cast<const DnsPacket*>(&pkt)) {
            for (IPacketListener* l : listeners_) l->on_dns(*p);
        }
    }

private:
    // ── PcapPlusPlus capture callback ───────────────────────────────────────

    static void on_packet_arrives(pcpp::RawPacket* raw,
                                   pcpp::PcapLiveDevice*,
                                   void* cookie) {
        auto* self = static_cast<PacketCapturer*>(cookie);
        NetPacket* pkt = parse(raw);
        if (pkt) {
            self->dispatch(*pkt);
            delete pkt;
        }
    }

    // ── Packet parsing ──────────────────────────────────────────────────────

    static NetPacket* parse(pcpp::RawPacket* raw) {
        pcpp::Packet parsed{raw};

        auto* ipv4 = parsed.getLayerOfType<pcpp::IPv4Layer>();
        if (!ipv4) return nullptr;

        const std::string src = ipv4->getSrcIPv4Address().toString();
        const std::string dst = ipv4->getDstIPv4Address().toString();
        const uint32_t    sz  = static_cast<uint32_t>(raw->getRawDataLen());
        const auto        ts  = std::chrono::system_clock::from_time_t(
                                    raw->getPacketTimeStamp().tv_sec);

        // DNS — check before generic TCP/UDP because DNS runs on port 53
        if (auto* dns = parsed.getLayerOfType<pcpp::DnsLayer>())
            return parse_dns(dns, src, dst, sz, ts);

        // HTTP request
        if (auto* req = parsed.getLayerOfType<pcpp::HttpRequestLayer>()) {
            auto* tcp = parsed.getLayerOfType<pcpp::TcpLayer>();
            return parse_http_request(req, tcp, src, dst, sz, ts);
        }

        // HTTP response
        if (auto* resp = parsed.getLayerOfType<pcpp::HttpResponseLayer>()) {
            auto* tcp = parsed.getLayerOfType<pcpp::TcpLayer>();
            return parse_http_response(resp, tcp, src, dst, sz, ts);
        }

        // Generic TCP
        if (auto* tcp = parsed.getLayerOfType<pcpp::TcpLayer>())
            return parse_tcp(tcp, src, dst, sz, ts);

        return nullptr;   // UDP/ICMP/etc. — not handled yet
    }

    // ── Per-protocol parsers ────────────────────────────────────────────────

    using TP = std::chrono::system_clock::time_point;

    static void fill_base(NetPacket& p,
                          const std::string& src, const std::string& dst,
                          uint32_t sz, TP ts) {
        p.src_ip    = src;
        p.dst_ip    = dst;
        p.size      = sz;
        p.timestamp = ts;
    }

    static void fill_tcp_fields(TcpPacket& p, pcpp::TcpLayer* tcp) {
        if (!tcp) return;
        auto* h       = tcp->getTcpHeader();
        p.src_port    = tcp->getSrcPort();
        p.dst_port    = tcp->getDstPort();
        p.seq_num     = ntohl(h->sequenceNumber);
        p.ack_num     = ntohl(h->ackNumber);
        p.window_size = ntohs(h->windowSize);
        p.syn = h->synFlag;
        p.ack = h->ackFlag;
        p.fin = h->finFlag;
        p.rst = h->rstFlag;
        p.psh = h->pshFlag;
    }

    static std::string http_method_str(pcpp::HttpRequestLayer::HttpMethod m) {
        switch (m) {
            case pcpp::HttpRequestLayer::HttpGET:     return "GET";
            case pcpp::HttpRequestLayer::HttpHEAD:    return "HEAD";
            case pcpp::HttpRequestLayer::HttpPOST:    return "POST";
            case pcpp::HttpRequestLayer::HttpPUT:     return "PUT";
            case pcpp::HttpRequestLayer::HttpDELETE:  return "DELETE";
            case pcpp::HttpRequestLayer::HttpTRACE:   return "TRACE";
            case pcpp::HttpRequestLayer::HttpOPTIONS: return "OPTIONS";
            case pcpp::HttpRequestLayer::HttpCONNECT: return "CONNECT";
            case pcpp::HttpRequestLayer::HttpPATCH:   return "PATCH";
            default:                                   return "UNKNOWN";
        }
    }

    static std::string http_version_str(pcpp::HttpVersion v) {
        switch (v) {
            case pcpp::OneDotOne:  return "HTTP/1.1";
            case pcpp::OneDotZero: return "HTTP/1.0";
            default:               return "HTTP/?";
        }
    }

    static std::string dns_type_str(pcpp::DnsType t) {
        switch (t) {
            case pcpp::DNS_TYPE_A:     return "A";
            case pcpp::DNS_TYPE_AAAA:  return "AAAA";
            case pcpp::DNS_TYPE_MX:    return "MX";
            case pcpp::DNS_TYPE_CNAME: return "CNAME";
            case pcpp::DNS_TYPE_PTR:   return "PTR";
            case pcpp::DNS_TYPE_TXT:   return "TXT";
            case pcpp::DNS_TYPE_NS:    return "NS";
            default: return "TYPE" + std::to_string(static_cast<int>(t));
        }
    }

    static NetPacket* parse_tcp(pcpp::TcpLayer* tcp,
                                 const std::string& src, const std::string& dst,
                                 uint32_t sz, TP ts) {
        auto* p = new TcpPacket{};
        fill_base(*p, src, dst, sz, ts);
        fill_tcp_fields(*p, tcp);
        return p;
    }

    static NetPacket* parse_http_request(pcpp::HttpRequestLayer* req,
                                          pcpp::TcpLayer* tcp,
                                          const std::string& src, const std::string& dst,
                                          uint32_t sz, TP ts) {
        auto* p = new HttpPacket{};
        fill_base(*p, src, dst, sz, ts);
        fill_tcp_fields(*p, tcp);
        p->is_response   = false;
        p->method        = http_method_str(req->getFirstLine()->getMethod());
        p->url           = req->getFirstLine()->getUri();
        p->http_version  = http_version_str(req->getFirstLine()->getVersion());
        if (auto* host = req->getFieldByName("Host"))
            p->host = host->getFieldValue();
        return p;
    }

    static NetPacket* parse_http_response(pcpp::HttpResponseLayer* resp,
                                           pcpp::TcpLayer* tcp,
                                           const std::string& src, const std::string& dst,
                                           uint32_t sz, TP ts) {
        auto* p = new HttpPacket{};
        fill_base(*p, src, dst, sz, ts);
        fill_tcp_fields(*p, tcp);
        p->is_response  = true;
        p->status_code  = resp->getFirstLine()->getStatusCodeAsInt();
        p->http_version = http_version_str(resp->getFirstLine()->getVersion());
        return p;
    }

    static NetPacket* parse_dns(pcpp::DnsLayer* dns,
                                 const std::string& src, const std::string& dst,
                                 uint32_t sz, TP ts) {
        auto* p = new DnsPacket{};
        fill_base(*p, src, dst, sz, ts);
        p->is_response = dns->getDnsHeader()->queryOrResponse;
        p->query_id    = ntohs(dns->getDnsHeader()->transactionID);
        if (auto* q = dns->getFirstQuery()) {
            p->domain      = q->getName();
            p->record_type = dns_type_str(q->getDnsType());
        }
        for (auto* a = dns->getFirstAnswer(); a; a = dns->getNextAnswer(a))
            if (auto data = a->getData()) p->answers.push_back(data->toString());
        return p;
    }

    // ── Data members ────────────────────────────────────────────────────────

    pcpp::PcapLiveDevice*      device_   = nullptr;
    std::vector<IPacketListener*> listeners_;
};
