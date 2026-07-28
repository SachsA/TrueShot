#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Net {

// ---------------------------------------------------------------------
// Wire-protocol common types. Pure POD-ish — no behaviour, no allocs.
// All serialisation lives in PacketTypes.h, which is the single owner
// of the byte layout.
// ---------------------------------------------------------------------

// Bump every time the wire format changes in a non-backwards-compatible
// way. The handshake refuses peers with a different protocol version.
constexpr uint8_t kProtocolVersion = 1;

// Default UDP port. Servers can be reconfigured to listen elsewhere.
constexpr uint16_t kDefaultPort = 7777;

// Reliable / unreliable channel split — see ADR-002.
constexpr uint8_t kChannelReliable   = 0;
constexpr uint8_t kChannelUnreliable = 1;
constexpr size_t kNumChannels        = 2;

// Maximum length the wire format permits for a player name (16 bytes,
// null-terminated, ASCII). Anything longer is truncated at the client.
constexpr uint8_t kMaxPlayerNameLen = 16;

// Hard ceiling for the number of entities in a Snapshot. Sized for
// 5v5 + spectators + grenades + dropped weapons.
constexpr uint8_t kMaxEntitiesPerSnapshot = 64;

// ---------------------------------------------------------------------
using Tick     = uint32_t;
using PlayerId = uint16_t; // 65 535 unique players per session: plenty.

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

// ----- Button bitfield used inside ClientInput. -----
namespace InputButton {
constexpr uint8_t Jump    = 1 << 0;
constexpr uint8_t Crouch  = 1 << 1;
constexpr uint8_t Fire    = 1 << 2;
constexpr uint8_t ADS     = 1 << 3;
constexpr uint8_t Reload  = 1 << 4;
constexpr uint8_t Walk    = 1 << 5; // shift-walk (silent)
constexpr uint8_t Use     = 1 << 6; // E — defuse, plant, pick-up
constexpr uint8_t Inspect = 1 << 7;
} // namespace InputButton

// ----- Entity flags used inside EntityState.stateFlags. -----
namespace EntityFlag {
constexpr uint8_t Alive     = 1 << 0;
constexpr uint8_t Crouching = 1 << 1;
constexpr uint8_t Firing    = 1 << 2;
constexpr uint8_t Reloading = 1 << 3;
constexpr uint8_t Aiming    = 1 << 4; // ADS
constexpr uint8_t OnGround  = 1 << 5;
} // namespace EntityFlag

// One frame's worth of player intent. Sent every tick by the client.
// Move and view are quantised on the wire; reconstructed as floats here.
struct InputState {
    Tick tick             = 0; // client local tick
    uint32_t seq          = 0; // monotonic; server ACKs in the Snapshot
    int8_t moveForward    = 0; // [-127, 127] — normalised to [-1, 1]
    int8_t moveRight      = 0;
    float yaw             = 0.0f; // degrees, [-180, +180]
    float pitch           = 0.0f; // degrees, [-90,  +90]
    uint8_t buttons       = 0;    // InputButton bitfield
    uint16_t clientPingMs = 0;    // last measured RTT, for lag comp
};

// One entity's state at a given tick. The server publishes one of these
// per replicated player / pickup / grenade.
struct EntityState {
    PlayerId id = 0;
    Vec3 pos{};
    float yaw          = 0.0f;
    float pitch        = 0.0f;
    uint8_t stateFlags = 0; // EntityFlag bitfield
};

// Snapshot — server's broadcast of the current world state.
struct Snapshot {
    Tick tick       = 0;
    uint32_t ackSeq = 0; // last ClientInput.seq applied
    std::vector<EntityState> entities;
};

// Handshake — first message client -> server on connect.
// Server replies with a HandshakeAck carrying the player's slot.
struct Handshake {
    uint8_t protocol                   = kProtocolVersion;
    uint8_t clientVerMaj               = 0;
    uint8_t clientVerMin               = 0;
    char playerName[kMaxPlayerNameLen] = {};
};

struct HandshakeAck {
    uint8_t protocol = kProtocolVersion;
    uint8_t accepted = 0; // 0 = reject, 1 = accept
    PlayerId slot    = 0; // assigned by server on accept
};

// Sent by either side to terminate the connection cleanly with a code.
struct Disconnect {
    uint8_t reason = 0;
};

} // namespace Net
