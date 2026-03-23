#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

#include <PcapFileDevice.h>

#include "capturer.hpp"

class PcapFileCapturer : public PacketCapturer {
public:
  static constexpr std::string_view type_name = "PcapFileCapturer";

  PcapFileCapturer()  = default;
  ~PcapFileCapturer() { close(); }

  PcapFileCapturer(const PcapFileCapturer &)            = delete;
  PcapFileCapturer &operator=(const PcapFileCapturer &) = delete;

  void open(std::string path) override {
    if (reader_.valid()) throw std::runtime_error{"already open"};

    reader_ = SmartPtr<pcpp::PcapFileReaderDevice, NoCopy>{new pcpp::PcapFileReaderDevice(path.c_str())};
    if (!reader_->open())
      throw std::runtime_error{"cannot open pcap file: " + path};

    pcpp::RawPacket raw;
    while (reader_->getNextPacket(raw)) {
      NetPacket *pkt = parse(&raw);
      if (pkt) {
        dispatch(*pkt);
        delete pkt;
      }
    }
  }

  void close() override {
    if (!reader_.valid()) return;
    reader_->close();
    reader_ = SmartPtr<pcpp::PcapFileReaderDevice, NoCopy>{};
  }

private:
  SmartPtr<pcpp::PcapFileReaderDevice, NoCopy> reader_;
};
