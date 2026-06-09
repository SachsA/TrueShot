// Bitstream round-trip tests. The wire format is the spine of every
// cross-OS exchange in TrueShot; if these regress, the netcode breaks
// silently between machines with different endianness or floating-
// point quirks.

#include "Network/Bitstream.h"

#include <gtest/gtest.h>

using namespace Net;

TEST(Bitstream, U8RoundTrip) {
    BitWriter bw;
    bw.writeU8(0);
    bw.writeU8(127);
    bw.writeU8(255);
    BitReader br(bw.buf.data(), bw.buf.size());
    uint8_t a = 0, b = 0, c = 0;
    EXPECT_TRUE(br.readU8(a));
    EXPECT_TRUE(br.readU8(b));
    EXPECT_TRUE(br.readU8(c));
    EXPECT_EQ(a, 0u);
    EXPECT_EQ(b, 127u);
    EXPECT_EQ(c, 255u);
}

TEST(Bitstream, U16LittleEndianExplicit) {
    BitWriter bw;
    bw.writeU16(0x1234);
    ASSERT_EQ(bw.buf.size(), 2u);
    EXPECT_EQ(bw.buf[0], 0x34); // low byte first
    EXPECT_EQ(bw.buf[1], 0x12);
}

TEST(Bitstream, U32RoundTrip) {
    BitWriter bw;
    bw.writeU32(0xDEADBEEFu);
    BitReader br(bw.buf.data(), bw.buf.size());
    uint32_t v = 0;
    EXPECT_TRUE(br.readU32(v));
    EXPECT_EQ(v, 0xDEADBEEFu);
}

TEST(Bitstream, FloatRoundTrip) {
    BitWriter bw;
    bw.writeFloat(1.234567f);
    bw.writeFloat(-9.87654e6f);
    BitReader br(bw.buf.data(), bw.buf.size());
    float a = 0, b = 0;
    EXPECT_TRUE(br.readFloat(a));
    EXPECT_TRUE(br.readFloat(b));
    EXPECT_FLOAT_EQ(a, 1.234567f);
    EXPECT_FLOAT_EQ(b, -9.87654e6f);
}

TEST(Bitstream, Q16_16RoundTripPrecision) {
    BitWriter bw;
    bw.writeQ16_16(123.456f);
    bw.writeQ16_16(-987.654f);
    BitReader br(bw.buf.data(), bw.buf.size());
    float a = 0, b = 0;
    EXPECT_TRUE(br.readQ16_16(a));
    EXPECT_TRUE(br.readQ16_16(b));
    // Q16.16 precision is 1/65536 ≈ 1.5e-5
    EXPECT_NEAR(a, 123.456f, 1.0f / 65536.0f);
    EXPECT_NEAR(b, -987.654f, 1.0f / 65536.0f);
}

TEST(Bitstream, AngleQ15RoundTripAt180Range) {
    BitWriter bw;
    bw.writeAngleQ15(45.0f, 180.0f);
    bw.writeAngleQ15(-179.9f, 180.0f);
    BitReader br(bw.buf.data(), bw.buf.size());
    float a = 0, b = 0;
    EXPECT_TRUE(br.readAngleQ15(a, 180.0f));
    EXPECT_TRUE(br.readAngleQ15(b, 180.0f));
    // Q15 at 180° range = ~0.0055° resolution
    EXPECT_NEAR(a, 45.0f, 0.01f);
    EXPECT_NEAR(b, -179.9f, 0.01f);
}

TEST(Bitstream, VarU32SmallValuesAreOneByte) {
    BitWriter bw;
    bw.writeVarU32(0);
    bw.writeVarU32(127);
    EXPECT_EQ(bw.buf.size(), 2u); // each fits in one byte
}

TEST(Bitstream, VarU32RoundTripLargeValues) {
    const uint32_t values[] = {0, 1, 127, 128, 16383, 16384, 0xFFFFFFFFu};
    BitWriter bw;
    for (auto v : values)
        bw.writeVarU32(v);
    BitReader br(bw.buf.data(), bw.buf.size());
    for (auto expected : values) {
        uint32_t got = 0;
        EXPECT_TRUE(br.readVarU32(got));
        EXPECT_EQ(got, expected);
    }
}

TEST(Bitstream, VarI32ZigzagPreservesSign) {
    const int32_t values[] = {0, 1, -1, 127, -128, 0x7FFFFFFF, -0x7FFFFFFF - 1};
    BitWriter bw;
    for (auto v : values)
        bw.writeVarI32(v);
    BitReader br(bw.buf.data(), bw.buf.size());
    for (auto expected : values) {
        int32_t got = 0;
        EXPECT_TRUE(br.readVarI32(got));
        EXPECT_EQ(got, expected);
    }
}

TEST(Bitstream, ReadOverflowReturnsFalse) {
    BitWriter bw;
    bw.writeU8(42);
    BitReader br(bw.buf.data(), bw.buf.size());
    uint8_t a  = 0;
    uint16_t b = 0;
    EXPECT_TRUE(br.readU8(a));
    EXPECT_FALSE(br.readU16(b)); // only 1 byte left, need 2
}
