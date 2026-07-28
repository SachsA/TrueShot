#pragma once

#include "netcode/bitstream.h"
#include "netcode/net_common.h"

#include <cstdint>
#include <cstring>

// ---------------------------------------------------------------------
// PacketTypes — TrueShot wire format.
//
// Every packet on the wire is:
//   uint8 PacketType
//   uint8 protocol version (kProtocolVersion)
//   <payload bytes…>
//
// The two header bytes give us a versioning contract: any peer reading
// a mismatched protocol immediately rejects the packet.
//
// All serialize/deserialize live here. The structs in NetCommon.h hold
// the data only; nothing in NetCommon depends on Bitstream.
//
// Functions return false on malformed input — callers MUST handle that.
// ---------------------------------------------------------------------

namespace Net {

enum class PacketType : uint8_t {
    // Connection lifecycle (channel 0, reliable)
    Handshake    = 0x10,
    HandshakeAck = 0x11,
    Disconnect   = 0x12,
    Ping         = 0x13,
    Pong         = 0x14,

    // Gameplay (channel 1, unreliable sequenced)
    ClientInput = 0x20,
    Snapshot    = 0x21,

    // Generic (channel 0, reliable)
    Event = 0x30,
    RPC   = 0x31,
};

// =====================================================================
// Header
// =====================================================================

inline void writeHeader(BitWriter& bw, PacketType type) {
    bw.writeU8(static_cast<uint8_t>(type));
    bw.writeU8(kProtocolVersion);
}

// Reads the leading two header bytes and validates the protocol version.
// On success, fills `outType`; on failure, returns false.
inline bool readHeader(BitReader& br, PacketType& outType) {
    uint8_t t = 0;
    uint8_t v = 0;
    if (!br.readU8(t)) return false;
    if (!br.readU8(v)) return false;
    if (v != kProtocolVersion) return false;
    outType = static_cast<PacketType>(t);
    return true;
}

// =====================================================================
// Handshake — client -> server, channel 0 (reliable)
// =====================================================================

inline void serialize(BitWriter& bw, const Handshake& h) {
    writeHeader(bw, PacketType::Handshake);
    bw.writeU8(h.clientVerMaj);
    bw.writeU8(h.clientVerMin);
    // Fixed-size 16-byte name field, null-padded.
    bw.write(h.playerName, kMaxPlayerNameLen);
}

inline bool deserializeBody(BitReader& br, Handshake& h) {
    if (!br.readU8(h.clientVerMaj)) return false;
    if (!br.readU8(h.clientVerMin)) return false;
    if (br.remaining < kMaxPlayerNameLen) return false;
    std::memcpy(h.playerName, br.p, kMaxPlayerNameLen);
    br.p += kMaxPlayerNameLen;
    br.remaining -= kMaxPlayerNameLen;
    // Force null-terminate the last byte defensively.
    h.playerName[kMaxPlayerNameLen - 1] = '\0';
    return true;
}

// =====================================================================
// HandshakeAck — server -> client, channel 0 (reliable)
// =====================================================================

inline void serialize(BitWriter& bw, const HandshakeAck& ack) {
    writeHeader(bw, PacketType::HandshakeAck);
    bw.writeU8(ack.accepted);
    bw.writeU16(ack.slot);
}

inline bool deserializeBody(BitReader& br, HandshakeAck& ack) {
    if (!br.readU8(ack.accepted)) return false;
    if (!br.readU16(ack.slot)) return false;
    return true;
}

// =====================================================================
// Disconnect — either side, channel 0
// =====================================================================

inline void serialize(BitWriter& bw, const Disconnect& d) {
    writeHeader(bw, PacketType::Disconnect);
    bw.writeU8(d.reason);
}

inline bool deserializeBody(BitReader& br, Disconnect& d) {
    return br.readU8(d.reason);
}

// =====================================================================
// ClientInput — client -> server, channel 1 (unreliable sequenced)
// =====================================================================
//
// Packet layout (bytes):
//   [0]   uint8   type   = 0x20
//   [1]   uint8   proto  = 1
//   [2-?] varint  tick           (1-5 bytes, usually 1-2)
//   [?]   varint  seq            (1-5 bytes)
//   [?]   int8    moveForward
//   [?]   int8    moveRight
//   [?]   Q15     yaw            (2 bytes)
//   [?]   Q15     pitch          (2 bytes)
//   [?]   uint8   buttons
//   [?]   uint16  clientPingMs
//
// Typical size on the wire: ~12-13 bytes after the 2-byte header.

inline void serialize(BitWriter& bw, const InputState& in) {
    writeHeader(bw, PacketType::ClientInput);
    bw.writeVarU32(in.tick);
    bw.writeVarU32(in.seq);
    bw.writeI8(in.moveForward);
    bw.writeI8(in.moveRight);
    bw.writeAngleQ15(in.yaw, 180.0f);
    bw.writeAngleQ15(in.pitch, 90.0f);
    bw.writeU8(in.buttons);
    bw.writeU16(in.clientPingMs);
}

inline bool deserializeBody(BitReader& br, InputState& in) {
    if (!br.readVarU32(in.tick)) return false;
    if (!br.readVarU32(in.seq)) return false;
    if (!br.readI8(in.moveForward)) return false;
    if (!br.readI8(in.moveRight)) return false;
    if (!br.readAngleQ15(in.yaw, 180.0f)) return false;
    if (!br.readAngleQ15(in.pitch, 90.0f)) return false;
    if (!br.readU8(in.buttons)) return false;
    if (!br.readU16(in.clientPingMs)) return false;
    return true;
}

// =====================================================================
// EntityState (used inside Snapshot — not a standalone packet)
// =====================================================================
//
// Per-entity wire layout: 17 bytes
//   uint16  id
//   Q16.16 × 3 (pos)         (12 bytes)
//   Q15      (yaw)            (2 bytes)
//   Q15      (pitch)          (2 bytes — wait, that's 18; recount below)
//
// Actually:
//   uint16   id           2
//   Q16.16x3 pos         12
//   Q15      yaw          2
//   Q15      pitch        2
//   uint8    stateFlags   1
//   ----------------------
//                  total 19 bytes

inline void writeEntityState(BitWriter& bw, const EntityState& e) {
    bw.writeU16(e.id);
    bw.writeVec3Q(e.pos.x, e.pos.y, e.pos.z);
    bw.writeAngleQ15(e.yaw, 180.0f);
    bw.writeAngleQ15(e.pitch, 90.0f);
    bw.writeU8(e.stateFlags);
}

inline bool readEntityState(BitReader& br, EntityState& e) {
    if (!br.readU16(e.id)) return false;
    if (!br.readVec3Q(e.pos.x, e.pos.y, e.pos.z)) return false;
    if (!br.readAngleQ15(e.yaw, 180.0f)) return false;
    if (!br.readAngleQ15(e.pitch, 90.0f)) return false;
    if (!br.readU8(e.stateFlags)) return false;
    return true;
}

// =====================================================================
// Snapshot — server -> client, channel 1 (unreliable sequenced)
// =====================================================================
//
// Packet layout:
//   uint8   type      = 0x21
//   uint8   proto     = 1
//   varint  tick
//   varint  ackSeq    (last ClientInput.seq applied for this peer)
//   uint8   entityCount        (clamped to kMaxEntitiesPerSnapshot)
//   repeat entityCount × EntityState (19 bytes each)
//
// Typical size: 5 + 19*10 = 195 bytes for a fully populated 10-player
// match. Well under any sane MTU.

inline void serialize(BitWriter& bw, const Snapshot& snap) {
    writeHeader(bw, PacketType::Snapshot);
    bw.writeVarU32(snap.tick);
    bw.writeVarU32(snap.ackSeq);

    const uint8_t count = static_cast<uint8_t>(snap.entities.size() > kMaxEntitiesPerSnapshot
                                                   ? kMaxEntitiesPerSnapshot
                                                   : snap.entities.size());
    bw.writeU8(count);
    for (uint8_t i = 0; i < count; ++i) {
        writeEntityState(bw, snap.entities[i]);
    }
}

inline bool deserializeBody(BitReader& br, Snapshot& snap) {
    if (!br.readVarU32(snap.tick)) return false;
    if (!br.readVarU32(snap.ackSeq)) return false;

    uint8_t count = 0;
    if (!br.readU8(count)) return false;
    if (count > kMaxEntitiesPerSnapshot) return false;

    snap.entities.resize(count);
    for (uint8_t i = 0; i < count; ++i) {
        if (!readEntityState(br, snap.entities[i])) return false;
    }
    return true;
}

} // namespace Net
