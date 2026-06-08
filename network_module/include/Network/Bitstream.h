#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

// -----------------------------------------------------------------------
// Bitstream — byte-oriented serialization for TrueShot's wire protocol.
//
// Design choices (see docs/adr/0002-netcode-architecture.md):
//
//   * Little-endian on the wire, always. The host endianness is never
//     trusted; we serialise byte-by-byte. Most modern targets ARE
//     little-endian (x86, arm64), so this is a no-op in practice, but
//     it future-proofs the protocol if we ever ship on big-endian.
//
//   * Floats and vectors are bit-cast to uint32 to avoid any platform
//     quirks (denormals, IEEE-754 representation variations).
//
//   * Fixed-point Q16.16 for positions / velocities — 1/65536-unit
//     precision, ±32k range. Deterministic across platforms, cheap to
//     pack, no float drift in delta compression.
//
//   * Q15 quantisation for view angles ([-180°, +180°] or [-90°, +90°]
//     mapped to int16) — 0.005° resolution, 2 bytes per angle.
//
//   * Varint (zigzag) for tick/seq counters that are usually small —
//     1 byte for values < 128, growing as needed.
//
// Reader returns false on every overflow so callers can reject malformed
// packets cleanly. NEVER memcpy a struct directly: alignment and padding
// would leak host details on the wire.
// -----------------------------------------------------------------------

namespace Net {

// ----- Forward-declared low-level primitives -----
namespace detail {

inline void writeU8(std::vector<uint8_t>& buf, uint8_t v) {
    buf.push_back(v);
}

inline void writeU16LE(std::vector<uint8_t>& buf, uint16_t v) {
    buf.push_back(static_cast<uint8_t>(v & 0xFFu));
    buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFFu));
}

inline void writeU32LE(std::vector<uint8_t>& buf, uint32_t v) {
    buf.push_back(static_cast<uint8_t>(v & 0xFFu));
    buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFFu));
    buf.push_back(static_cast<uint8_t>((v >> 16) & 0xFFu));
    buf.push_back(static_cast<uint8_t>((v >> 24) & 0xFFu));
}

inline void writeU64LE(std::vector<uint8_t>& buf, uint64_t v) {
    for (int i = 0; i < 8; ++i) {
        buf.push_back(static_cast<uint8_t>((v >> (i * 8)) & 0xFFu));
    }
}

inline uint16_t readU16LE(const uint8_t* p) {
    return static_cast<uint16_t>(p[0]) | static_cast<uint16_t>(static_cast<uint16_t>(p[1]) << 8);
}

inline uint32_t readU32LE(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

inline uint64_t readU64LE(const uint8_t* p) {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) {
        v |= static_cast<uint64_t>(p[i]) << (i * 8);
    }
    return v;
}

// ----- IEEE-754 bit casts (no std::bit_cast on C++17) -----
inline uint32_t bitcastFloatToU32(float f) {
    uint32_t u = 0;
    std::memcpy(&u, &f, sizeof(u));
    return u;
}
inline float bitcastU32ToFloat(uint32_t u) {
    float f = 0.0f;
    std::memcpy(&f, &u, sizeof(f));
    return f;
}

} // namespace detail

// =======================================================================
// BitWriter — append-only byte buffer, little-endian on the wire.
// =======================================================================
struct BitWriter {
    std::vector<uint8_t> buf;

    BitWriter()                            = default;
    BitWriter(const BitWriter&)            = delete;
    BitWriter& operator=(const BitWriter&) = delete;
    BitWriter(BitWriter&&)                 = default;
    BitWriter& operator=(BitWriter&&)      = default;

    // Reserve capacity in bytes to avoid reallocations on large packets.
    void reserve(size_t bytes) { buf.reserve(bytes); }

    // Raw byte append — escape hatch for blobs (strings, audio frames).
    void write(const void* data, size_t s) {
        const uint8_t* p = static_cast<const uint8_t*>(data);
        buf.insert(buf.end(), p, p + s);
    }

    // ----- Fixed-width little-endian integers -----
    void writeU8(uint8_t v) { detail::writeU8(buf, v); }
    void writeU16(uint16_t v) { detail::writeU16LE(buf, v); }
    void writeU32(uint32_t v) { detail::writeU32LE(buf, v); }
    void writeU64(uint64_t v) { detail::writeU64LE(buf, v); }

    void writeI8(int8_t v) { writeU8(static_cast<uint8_t>(v)); }
    void writeI16(int16_t v) { writeU16(static_cast<uint16_t>(v)); }
    void writeI32(int32_t v) { writeU32(static_cast<uint32_t>(v)); }
    void writeI64(int64_t v) { writeU64(static_cast<uint64_t>(v)); }

    // ----- Floating point (IEEE-754, little-endian bit pattern) -----
    void writeFloat(float v) { writeU32(detail::bitcastFloatToU32(v)); }

    // ----- Fixed-point Q16.16 — for positions / velocities -----
    // Range: ±32767.999..., precision: 1/65536 ≈ 1.5e-5
    // 4 bytes on the wire, deterministic across platforms.
    void writeQ16_16(float v) {
        // Saturate before truncation so we never get UB on overflow.
        const double scaled  = static_cast<double>(v) * 65536.0;
        const double clamped = scaled < -2147483648.0  ? -2147483648.0
                               : scaled > 2147483647.0 ? 2147483647.0
                                                       : scaled;
        writeI32(static_cast<int32_t>(clamped));
    }

    // ----- Q15 angle — for yaw [-180°, +180°] or pitch [-90°, +90°] -----
    // Pass the absolute angle range max in degrees (180 or 90).
    // Resolution ≈ rangeMax / 32767. For yaw=180 → 0.0055°, more than
    // enough for any flick the player can perform.
    void writeAngleQ15(float angleDeg, float rangeMax) {
        const float clamped = angleDeg < -rangeMax  ? -rangeMax
                              : angleDeg > rangeMax ? rangeMax
                                                    : angleDeg;
        const int32_t q =
            static_cast<int32_t>(std::lround(static_cast<double>(clamped) / rangeMax * 32767.0));
        writeI16(static_cast<int16_t>(q));
    }

    // ----- 3D vectors (3 × Q16.16 = 12 bytes) -----
    void writeVec3Q(float x, float y, float z) {
        writeQ16_16(x);
        writeQ16_16(y);
        writeQ16_16(z);
    }

    // ----- Variable-length integers (zigzag for signed) -----
    // Useful for tick / seq counters that are usually small; 1 byte for
    // values < 128, growing as needed. Saves bytes vs fixed-width on
    // hot-path packets (ClientInput) where every byte counts.
    void writeVarU32(uint32_t v) {
        while (v >= 0x80) {
            detail::writeU8(buf, static_cast<uint8_t>(v | 0x80));
            v >>= 7;
        }
        detail::writeU8(buf, static_cast<uint8_t>(v));
    }
    void writeVarI32(int32_t v) {
        const uint32_t zz = (static_cast<uint32_t>(v) << 1) ^ static_cast<uint32_t>(v >> 31);
        writeVarU32(zz);
    }

    // ----- Length-prefixed string (uint16 length + bytes) -----
    void writeString(const std::string& s) {
        const uint16_t n = static_cast<uint16_t>(s.size());
        writeU16(n);
        if (n != 0) write(s.data(), n);
    }

    // -------------------------------------------------------------------
    // LEGACY: raw POD write (host endianness, no padding control).
    //
    // Kept only for the original Server.cpp / Client.cpp prototypes
    // until Phase 1.3 rewrites them on top of the typed API above.
    // Do NOT use in new code — it leaks host alignment / endianness
    // onto the wire and will break the moment a non-x86 client joins.
    // -------------------------------------------------------------------
    template <typename T>
    [[deprecated("use writeU32 / writeFloat / writeQ16_16 / etc. instead")]]
    void writePOD(const T& v) {
        write(&v, sizeof(T));
    }
};

// =======================================================================
// BitReader — bounds-checked read cursor over an immutable byte buffer.
// Every read() returns false on overflow; the caller is expected to
// abort the packet on first failure.
// =======================================================================
struct BitReader {
    const uint8_t* p;
    size_t remaining;

    BitReader(const uint8_t* data, size_t len) : p(data), remaining(len) {}

    bool read(void* out, size_t s) {
        if (s > remaining) return false;
        std::memcpy(out, p, s);
        p += s;
        remaining -= s;
        return true;
    }

    // ----- Fixed-width integers -----
    bool readU8(uint8_t& out) {
        if (remaining < 1) return false;
        out = p[0];
        p += 1;
        remaining -= 1;
        return true;
    }
    bool readU16(uint16_t& out) {
        if (remaining < 2) return false;
        out = detail::readU16LE(p);
        p += 2;
        remaining -= 2;
        return true;
    }
    bool readU32(uint32_t& out) {
        if (remaining < 4) return false;
        out = detail::readU32LE(p);
        p += 4;
        remaining -= 4;
        return true;
    }
    bool readU64(uint64_t& out) {
        if (remaining < 8) return false;
        out = detail::readU64LE(p);
        p += 8;
        remaining -= 8;
        return true;
    }

    bool readI8(int8_t& out) {
        uint8_t u = 0;
        if (!readU8(u)) return false;
        out = static_cast<int8_t>(u);
        return true;
    }
    bool readI16(int16_t& out) {
        uint16_t u = 0;
        if (!readU16(u)) return false;
        out = static_cast<int16_t>(u);
        return true;
    }
    bool readI32(int32_t& out) {
        uint32_t u = 0;
        if (!readU32(u)) return false;
        out = static_cast<int32_t>(u);
        return true;
    }
    bool readI64(int64_t& out) {
        uint64_t u = 0;
        if (!readU64(u)) return false;
        out = static_cast<int64_t>(u);
        return true;
    }

    bool readFloat(float& out) {
        uint32_t u = 0;
        if (!readU32(u)) return false;
        out = detail::bitcastU32ToFloat(u);
        return true;
    }

    // ----- Fixed-point Q16.16 -----
    bool readQ16_16(float& out) {
        int32_t q = 0;
        if (!readI32(q)) return false;
        out = static_cast<float>(q) / 65536.0f;
        return true;
    }

    // ----- Q15 angle (rangeMax = 180 for yaw, 90 for pitch) -----
    bool readAngleQ15(float& outDeg, float rangeMax) {
        int16_t q = 0;
        if (!readI16(q)) return false;
        outDeg = static_cast<float>(q) * rangeMax / 32767.0f;
        return true;
    }

    // ----- 3D vector (3 × Q16.16) -----
    bool readVec3Q(float& x, float& y, float& z) {
        return readQ16_16(x) && readQ16_16(y) && readQ16_16(z);
    }

    // ----- Variable-length integers -----
    bool readVarU32(uint32_t& out) {
        uint32_t v = 0;
        int shift  = 0;
        while (true) {
            uint8_t b = 0;
            if (!readU8(b)) return false;
            v |= static_cast<uint32_t>(b & 0x7F) << shift;
            if ((b & 0x80) == 0) {
                out = v;
                return true;
            }
            shift += 7;
            if (shift >= 35) return false; // malformed: > 5 bytes
        }
    }
    bool readVarI32(int32_t& out) {
        uint32_t zz = 0;
        if (!readVarU32(zz)) return false;
        out = static_cast<int32_t>((zz >> 1) ^ -static_cast<int32_t>(zz & 1));
        return true;
    }

    // ----- Length-prefixed string -----
    bool readString(std::string& s) {
        uint16_t n = 0;
        if (!readU16(n)) return false;
        if (n == 0) {
            s.clear();
            return true;
        }
        if (n > remaining) return false;
        s.assign(reinterpret_cast<const char*>(p), n);
        p += n;
        remaining -= n;
        return true;
    }

    // -------------------------------------------------------------------
    // LEGACY: raw POD read (host endianness, no padding control).
    // See note on BitWriter::writePOD — slated for removal in Phase 1.3.
    // -------------------------------------------------------------------
    template <typename T>
    [[deprecated("use readU32 / readFloat / readQ16_16 / etc. instead")]]
    bool readPOD(T& out) {
        return read(&out, sizeof(T));
    }
};

} // namespace Net
