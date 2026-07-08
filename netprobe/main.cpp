import loki23;
import loki23.factory;

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "listener.hpp"
#include "live_capturer.hpp"
#include "pcap_file_capturer.hpp"
#include "smart_listener.hpp"
#include "statistics_listener.hpp"

using Listeners = type_list<PacketLogger, StatisticsListener, SmartListener>;
using Capturers = type_list<LiveCapturer, PcapFileCapturer>;

int main() {
  HierarchyFactory<PacketListener, Listeners> listener_factory;
  SmartPtr<PacketListener, NoCopy> stats  = Field<StatisticsListener>(listener_factory).create();
  SmartPtr<PacketListener, NoCopy> smart  = Field<SmartListener>(listener_factory).create();

  std::cout << "Select source:\n"
            << "  1) Live interface\n"
            << "  2) Pcap file\n"
            << "> ";
  std::string choice;
  if (!std::getline(std::cin, choice)) return 1;

  HierarchyFactory<PacketCapturer, Capturers> capturer_factory;
  SmartPtr<PacketCapturer, NoCopy> capturer;
  std::string target;

  if (choice == "1") {
    capturer = Field<LiveCapturer>(capturer_factory).create();
    std::cout << "Interface name (empty = first available): ";
    std::getline(std::cin, target);
  } else if (choice == "2") {
    capturer = Field<PcapFileCapturer>(capturer_factory).create();
    std::cout << "Pcap file path: ";
    std::getline(std::cin, target);
  } else {
    std::cerr << "Invalid choice: " << choice << "\n";
    return 1;
  }

  capturer->add_listener(stats.get());
  capturer->add_listener(smart.get());

  if (choice == "1") {
    capturer->open(target);
    auto *live = static_cast<LiveCapturer *>(capturer.get());
    std::cout << "Capturing on " << live->interface_name() << " for 50 seconds...\n";
    std::this_thread::sleep_for(std::chrono::seconds{50});
    capturer->close();
  } else {
    capturer->open(target);
  }

  static_cast<StatisticsListener *>(stats.get())->print_report(std::cout);
  std::cout << "Capture done.\n";
  return 0;
}
