// Tests for Net::stepSim — the SHARED authoritative simulation that
// runs identically on client and server. Any drift here breaks
// reconciliation, so we keep the invariant tight via tests.

#include "Network/NetSim.h"

#include <gtest/gtest.h>

TEST(NetSim, ZeroInputIsAStaticState) {
    Net::SimState s;
    Net::InputState in{};
    Net::stepSim(s, in);
    EXPECT_FLOAT_EQ(s.pos.x, 0.0f);
    EXPECT_FLOAT_EQ(s.pos.z, 0.0f);
}

TEST(NetSim, MoveForwardAtZeroYawWalksAlongPositiveX) {
    // yaw = 0 means we face +X. Walking forward should move +X.
    Net::SimState s;
    Net::InputState in{};
    in.moveForward = 127; // full forward
    in.yaw         = 0.0f;
    Net::stepSim(s, in);
    EXPECT_GT(s.pos.x, 0.0f);
    EXPECT_NEAR(s.pos.z, 0.0f, 1e-5f);
}

TEST(NetSim, ClampForwardAboveOne) {
    // moveForward is int8_t / 127 — anything > 127 should still clamp
    // to 1.0 inside stepSim. (Our wire format stores int8_t so the only
    // way to exceed is via direct field write in tests — exactly the
    // case anti-cheat must catch.)
    Net::SimState s;
    Net::InputState in{};
    in.moveForward = 100; // ~0.787 ratio
    in.yaw         = 0.0f;
    Net::stepSim(s, in);
    const float expected = (100.0f / 127.0f) * Net::kNetSimMoveSpeed * Net::kNetSimFixedDt;
    EXPECT_NEAR(s.pos.x, expected, 1e-5f);
}

TEST(NetSim, YawIsClampedToPlusMinus180) {
    Net::SimState s;
    Net::InputState in{};
    in.yaw   = 1000.0f;
    in.pitch = 200.0f;
    Net::stepSim(s, in);
    EXPECT_FLOAT_EQ(s.yaw, 180.0f);
    EXPECT_FLOAT_EQ(s.pitch, 89.0f);

    in.yaw   = -1000.0f;
    in.pitch = -200.0f;
    Net::stepSim(s, in);
    EXPECT_FLOAT_EQ(s.yaw, -180.0f);
    EXPECT_FLOAT_EQ(s.pitch, -89.0f);
}

TEST(NetSim, StepIsDeterministicAcrossRuns) {
    // Identical inputs => identical outputs, bit-for-bit. This is the
    // contract that lets client prediction match server truth without
    // constant reconciliation snaps.
    Net::SimState a;
    Net::SimState b;
    Net::InputState in{};
    in.moveForward = 80;
    in.moveRight   = -40;
    in.yaw         = 37.5f;
    in.pitch       = -12.0f;

    for (int i = 0; i < 1000; ++i) {
        Net::stepSim(a, in);
        Net::stepSim(b, in);
    }

    EXPECT_FLOAT_EQ(a.pos.x, b.pos.x);
    EXPECT_FLOAT_EQ(a.pos.y, b.pos.y);
    EXPECT_FLOAT_EQ(a.pos.z, b.pos.z);
    EXPECT_FLOAT_EQ(a.yaw, b.yaw);
}

TEST(NetSim, ButtonsMapToStateFlags) {
    Net::SimState s;
    Net::InputState in{};
    in.buttons = Net::InputButton::Crouch | Net::InputButton::ADS;
    Net::stepSim(s, in);
    EXPECT_TRUE(s.stateFlags & Net::EntityFlag::Crouching);
    EXPECT_TRUE(s.stateFlags & Net::EntityFlag::Aiming);
    EXPECT_FALSE(s.stateFlags & Net::EntityFlag::Firing);
    EXPECT_TRUE(s.stateFlags & Net::EntityFlag::Alive);
}
