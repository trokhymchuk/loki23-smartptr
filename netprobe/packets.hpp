#pragma once

#include <arpa/inet.h>
#include <chrono>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

#include <DnsLayer.h>
#include <HttpLayer.h>
#include <IPv4Layer.h>
#include <Packet.h>
#include <RawPacket.h>
#include <TcpLayer.h>
#include <UdpLayer.h>

struct NetPacket {
    std::string src_ip;
    std::string dst_ip;
    uint32_t    size      = 0;
    std::chrono::system_clock::time_point timestamp;

    virtual ~NetPacket() = default;

    virtual std::string type_name() const = 0;
    virtual NetPacket*  deep_copy()  const = 0;

    virtual std::string to_string() const {
        auto t  = std::chrono::system_clock::to_time_t(timestamp);
        auto tm = *std::localtime(&t);
        char buf[20];
        std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm);

        std::ostringstream os;
        os << "[" << buf << "] "
           << type_name() << "  "
           << src_ip << " -> " << dst_ip
           << "  (" << size << " B)";
        return os.str();
    }

protected:
    void fill_base(pcpp::IPv4Layer* ipv4, pcpp::RawPacket* raw) {
        src_ip    = ipv4->getSrcIPv4Address().toString();
        dst_ip    = ipv4->getDstIPv4Address().toString();
        size      = static_cast<uint32_t>(raw->getRawDataLen());
        timestamp = std::chrono::system_clock::from_time_t(
                        raw->getPacketTimeStamp().tv_sec);
    }
};

inline std::ostream& operator<<(std::ostream& os, const NetPacket& p) {
    return os << p.to_string();
}

struct TcpPacket : NetPacket {
    uint16_t src_port    = 0;
    uint16_t dst_port    = 0;
    uint32_t seq_num     = 0;
    uint32_t ack_num     = 0;
    uint16_t window_size = 0;

    bool syn = false;
    bool ack = false;
    bool fin = false;
    bool rst = false;
    bool psh = false;

    TcpPacket() = default;

    explicit TcpPacket(const pcpp::Packet& parsed, pcpp::RawPacket* raw) {
        auto* ipv4 = parsed.getLayerOfType<pcpp::IPv4Layer>();
        auto* tcp  = parsed.getLayerOfType<pcpp::TcpLayer>();
        if (!ipv4 || !tcp)
            throw std::invalid_argument{"not an IPv4/TCP packet"};

        fill_base(ipv4, raw);

        auto* h   = tcp->getTcpHeader();
        src_port  = tcp->getSrcPort();
        dst_port  = tcp->getDstPort();
        seq_num   = ntohl(h->sequenceNumber);
        ack_num   = ntohl(h->ackNumber);
        window_size = ntohs(h->windowSize);
        syn = h->synFlag;
        ack = h->ackFlag;
        fin = h->finFlag;
        rst = h->rstFlag;
        psh = h->pshFlag;
    }

    std::string flags_str() const {
        std::string f;
        if (syn) f += "SYN|";
        if (ack) f += "ACK|";
        if (fin) f += "FIN|";
        if (rst) f += "RST|";
        if (psh) f += "PSH|";
        if (!f.empty()) f.pop_back();
        return f.empty() ? "-" : f;
    }

    std::string type_name() const override { return "TCP"; }

    NetPacket* deep_copy() const override { return new TcpPacket(*this); }

    std::string to_string() const override {
        std::ostringstream os;
        os << NetPacket::to_string() << "\n"
           << "        ports: " << src_port << " -> " << dst_port
           << "  seq=" << seq_num << "  ack=" << ack_num
           << "  flags=[" << flags_str() << "]"
           << "  win=" << window_size;
        return os.str();
    }
};

inline std::ostream& operator<<(std::ostream& os, const TcpPacket& p) {
    return os << p.to_string();
}

struct HttpPacket : TcpPacket {
    std::string method;
    std::string url;
    std::string http_version;
    std::string host;
    int         status_code = 0;
    bool        is_response = false;

    std::unordered_map<std::string, std::string> headers;

    HttpPacket() = default;

    explicit HttpPacket(const pcpp::Packet& parsed, pcpp::RawPacket* raw)
        : TcpPacket{parsed, raw}
    {
        auto* req  = parsed.getLayerOfType<pcpp::HttpRequestLayer>();
        auto* resp = parsed.getLayerOfType<pcpp::HttpResponseLayer>();
        if (!req && !resp)
            throw std::invalid_argument{"not an HTTP packet"};

        if (req) {
            is_response  = false;
            method       = http_method_str(req->getFirstLine()->getMethod());
            url          = req->getFirstLine()->getUri();
            http_version = http_version_str(req->getFirstLine()->getVersion());
            if (auto* h = req->getFieldByName("Host"))
                host = h->getFieldValue();
        } else {
            is_response  = true;
            status_code  = resp->getFirstLine()->getStatusCodeAsInt();
            http_version = http_version_str(resp->getFirstLine()->getVersion());
        }
    }

    std::string type_name() const override { return "HTTP"; }

    NetPacket* deep_copy() const override { return new HttpPacket(*this); }

    std::string to_string() const override {
        std::ostringstream os;
        os << TcpPacket::to_string() << "\n";
        if (is_response) {
            os << "        HTTP response  status=" << status_code
               << "  version=" << http_version;
        } else {
            os << "        HTTP request   " << method << " " << url
               << "  version=" << http_version
               << "  host=" << host;
        }
        return os.str();
    }

private:
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
};

inline std::ostream& operator<<(std::ostream& os, const HttpPacket& p) {
    return os << p.to_string();
}

struct DnsPacket : NetPacket {
    uint16_t    query_id    = 0;
    std::string domain;
    std::string record_type;
    bool        is_response = false;

    std::vector<std::string> answers;

    DnsPacket() = default;

    explicit DnsPacket(const pcpp::Packet& parsed, pcpp::RawPacket* raw) {
        auto* ipv4 = parsed.getLayerOfType<pcpp::IPv4Layer>();
        auto* dns  = parsed.getLayerOfType<pcpp::DnsLayer>();
        if (!ipv4 || !dns)
            throw std::invalid_argument{"not an IPv4/DNS packet"};

        fill_base(ipv4, raw);

        is_response = dns->getDnsHeader()->queryOrResponse;
        query_id    = ntohs(dns->getDnsHeader()->transactionID);
        if (auto* q = dns->getFirstQuery()) {
            domain      = q->getName();
            record_type = dns_type_str(q->getDnsType());
        }
        for (auto* a = dns->getFirstAnswer(); a; a = dns->getNextAnswer(a))
            if (auto data = a->getData()) answers.push_back(data->toString());
    }

    std::string type_name() const override { return "DNS"; }

    NetPacket* deep_copy() const override { return new DnsPacket(*this); }

    std::string to_string() const override {
        std::ostringstream os;
        os << NetPacket::to_string() << "\n"
           << "        DNS " << (is_response ? "response" : "query")
           << "  id=0x" << std::hex << query_id << std::dec
           << "  " << record_type << " " << domain;
        if (is_response && !answers.empty()) {
            os << "  -> [";
            for (std::size_t i = 0; i < answers.size(); ++i) {
                if (i) os << ", ";
                os << answers[i];
            }
            os << "]";
        }
        return os.str();
    }

private:
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
};

inline std::ostream& operator<<(std::ostream& os, const DnsPacket& p) {
    return os << p.to_string();
}

struct UdpPacket : NetPacket {
    uint16_t src_port = 0;
    uint16_t dst_port = 0;

    UdpPacket() = default;

    explicit UdpPacket(const pcpp::Packet& parsed, pcpp::RawPacket* raw) {
        auto* ipv4 = parsed.getLayerOfType<pcpp::IPv4Layer>();
        auto* udp  = parsed.getLayerOfType<pcpp::UdpLayer>();
        if (!ipv4 || !udp)
            throw std::invalid_argument{"not an IPv4/UDP packet"};

        fill_base(ipv4, raw);
        src_port = udp->getSrcPort();
        dst_port = udp->getDstPort();
    }

    std::string type_name() const override { return "UDP"; }

    NetPacket* deep_copy() const override { return new UdpPacket(*this); }

    std::string to_string() const override {
        std::ostringstream os;
        os << NetPacket::to_string() << "\n"
           << "        ports: " << src_port << " -> " << dst_port;
        return os.str();
    }
};

inline std::ostream& operator<<(std::ostream& os, const UdpPacket& p) {
    return os << p.to_string();
}
