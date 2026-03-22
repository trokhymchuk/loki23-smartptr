#pragma once

#include <chrono>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// Base class — IP-level fields common to every captured packet
// ─────────────────────────────────────────────────────────────────────────────
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
};

inline std::ostream& operator<<(std::ostream& os, const NetPacket& p) {
    return os << p.to_string();
}

// ─────────────────────────────────────────────────────────────────────────────
// TcpPacket — transport layer: ports, sequence numbers, control flags
// ─────────────────────────────────────────────────────────────────────────────
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

    std::string flags_str() const {
        std::string f;
        if (syn) f += "SYN|";
        if (ack) f += "ACK|";
        if (fin) f += "FIN|";
        if (rst) f += "RST|";
        if (psh) f += "PSH|";
        if (!f.empty()) f.pop_back();   // trim trailing '|'
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

// ─────────────────────────────────────────────────────────────────────────────
// HttpPacket — application layer over TCP: method/URL or status line, headers
// ─────────────────────────────────────────────────────────────────────────────
struct HttpPacket : TcpPacket {
    // For requests:  method + url  (status_code == 0, is_response == false)
    // For responses: status_code   (method is empty,  is_response == true)
    std::string method;       // GET, POST, PUT, DELETE, HEAD, …
    std::string url;          // /index.html?q=foo
    std::string http_version; // HTTP/1.0 | HTTP/1.1 | HTTP/2
    std::string host;         // Host header value
    int         status_code  = 0;
    bool        is_response  = false;

    std::unordered_map<std::string, std::string> headers;

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
};

inline std::ostream& operator<<(std::ostream& os, const HttpPacket& p) {
    return os << p.to_string();
}

// ─────────────────────────────────────────────────────────────────────────────
// DnsPacket — application layer (UDP/53 or TCP/53): query and response records
// ─────────────────────────────────────────────────────────────────────────────
struct DnsPacket : NetPacket {
    uint16_t    query_id    = 0;
    std::string domain;        // e.g. "example.com"
    std::string record_type;   // A, AAAA, MX, CNAME, PTR, TXT, …
    bool        is_response = false;

    // Populated on responses — resolved IPs or CNAME targets
    std::vector<std::string> answers;

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
};

inline std::ostream& operator<<(std::ostream& os, const DnsPacket& p) {
    return os << p.to_string();
}
