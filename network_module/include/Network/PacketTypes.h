#pragma once
#include <cstdint>

namespace Net {

enum class PacketType : uint8_t {
    ClientInput = 0x01,
    Snapshot    = 0x02,
    Event       = 0x03,
    RPC         = 0x04
};

}
