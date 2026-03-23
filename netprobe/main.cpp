import loki23;
import loki23.factory;

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "listener.hpp"
#include "statistics_listener.hpp"
#include "smart_listener.hpp"
#include "live_capturer.hpp"
#include "pcap_file_capturer.hpp"

using Listeners = type_list<PacketLogger, StatisticsListener, SmartListener>;
using Capturers = type_list<LiveCapturer, PcapFileCapturer>;

int main(int argc, char* argv[]) {
  HierarchyFactory<PacketListener, Listeners> listener_factory;
  SmartPtr<PacketListener, NoCopy> logger    {Field<PacketLogger>        (listener_factory).create().release()};
  SmartPtr<PacketListener, NoCopy> stats     {Field<StatisticsListener>  (listener_factory).create().release()};
  SmartPtr<PacketListener, NoCopy> smart     {Field<SmartListener>       (listener_factory).create().release()};

  HierarchyFactory<PacketCapturer, Capturers> capturer_factory;
  SmartPtr<PacketCapturer, NoCopy> capturer;
  bool is_live = true;

  if (argc >= 3 && std::string{argv[1]} == "-f") {
    capturer = SmartPtr<PacketCapturer, NoCopy>{Field<PcapFileCapturer>(capturer_factory).create().release()};
    is_live  = false;
  } else {
    capturer = SmartPtr<PacketCapturer, NoCopy>{Field<LiveCapturer>(capturer_factory).create().release()};
  }

//   capturer->add_listener(logger.get());
  capturer->add_listener(stats.get());
  capturer->add_listener(smart.get());

  if (is_live) {
    const std::string iface =
        (argc >= 3 && std::string{argv[1]} == "-i") ? argv[2] : "";
    capturer->open(iface);

    auto* live = static_cast<LiveCapturer*>(capturer.get());
    std::cout << "Capturing on " << live->interface_name() << " for 50 seconds...\n";
    std::this_thread::sleep_for(std::chrono::seconds{50});
    capturer->close();
  } else {
    capturer->open(argv[2]);
  }

  static_cast<StatisticsListener*>(stats.get())->print_report(std::cout);
  std::cout << "Capture done.\n";
  return 0;
}
