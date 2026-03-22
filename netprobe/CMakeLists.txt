include(FetchContent)

FetchContent_Declare(
    PcapPlusPlus
    GIT_REPOSITORY https://github.com/seladb/PcapPlusPlus.git
    GIT_TAG        master # Or specify a specific release tag like v23.09
)

FetchContent_MakeAvailable(PcapPlusPlus)

# find_package(PcapPlusPlus REQUIRED)

add_executable(netprobe main.cpp $<TARGET_OBJECTS:loki23>)

target_link_libraries(netprobe PRIVATE Packet++ Pcap++)