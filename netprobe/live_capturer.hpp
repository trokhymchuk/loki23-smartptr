#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

#include <PcapLiveDevice.h>
#include <PcapLiveDeviceList.h>

#include "capturer.hpp"

class LiveCapturer : public PacketCapturer {
public:
  static constexpr std::string_view type_name = "LiveCapturer";

  LiveCapturer()  = default;
  ~LiveCapturer() { close(); }

  LiveCapturer(const LiveCapturer &)            = delete;
  LiveCapturer &operator=(const LiveCapturer &) = delete;

  void open(std::string interface_name = {}) override {
    if (device_) throw std::runtime_error{"already open"};

    if (interface_name.empty()) {
      auto &devices = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDevicesList();
      if (devices.empty())
        throw std::runtime_error{"no network interfaces found"};
      device_         = devices.front();
      interface_name_ = device_->getName();
    } else {
      device_ = pcpp::PcapLiveDeviceList::getInstance().getDeviceByName(interface_name);
      if (!device_)
        throw std::runtime_error{"interface not found: " + interface_name};
      interface_name_ = std::move(interface_name);
    }

    if (!device_->open())
      throw std::runtime_error{"cannot open interface: " + interface_name_};

    device_->startCapture(on_packet_arrives, this);
  }

  void close() override {
    if (!device_) return;
    device_->stopCapture();
    device_->close();
    device_ = nullptr;
  }

  const std::string &interface_name() const { return interface_name_; }

private:
  static void on_packet_arrives(pcpp::RawPacket *raw, pcpp::PcapLiveDevice *, void *cookie) {
    auto *self = static_cast<LiveCapturer *>(cookie);
    NetPacket *pkt = parse(raw);
    if (pkt) {
      self->dispatch(*pkt);
      delete pkt;
    }
  }

  std::string           interface_name_;
  pcpp::PcapLiveDevice *device_ = nullptr;
};
