// PlayerHistory tests — the server-side rewind buffer used by lag
// compensation. The interpolation math must be tight; if it isn't, hit
// registration silently mis-aligns.

#include "netcode/lag_compensation.h"

#include <gtest/gtest.h>

using namespace Net;

namespace {
HistorySample make(double t, float x, float y = 0.0f, float yaw = 0.0f) {
    HistorySample s;
    s.tServer = t;
    s.pos     = glm::vec3(x, y, 0.0f);
    s.yaw     = yaw;
    return s;
}
} // namespace

TEST(PlayerHistory, EmptyBufferRefusesSample) {
    PlayerHistory h;
    glm::vec3 pos;
    float yaw = 0;
    EXPECT_FALSE(h.sampleAt(0.0, pos, yaw));
}

TEST(PlayerHistory, SingleSampleEchoesPastQuery) {
    PlayerHistory h;
    h.push(make(1.0, 5.0f));
    glm::vec3 pos;
    float yaw = 0;
    // tTarget == sample.tServer counts as "newer" branch — pin to it.
    EXPECT_TRUE(h.sampleAt(1.0, pos, yaw));
    EXPECT_FLOAT_EQ(pos.x, 5.0f);
}

TEST(PlayerHistory, InterpolatesMidpointBetweenTwoSamples) {
    PlayerHistory h;
    h.push(make(1.0, 0.0f, 0.0f, 0.0f));
    h.push(make(2.0, 10.0f, 0.0f, 90.0f));
    glm::vec3 pos;
    float yaw = 0;
    EXPECT_TRUE(h.sampleAt(1.5, pos, yaw));
    EXPECT_NEAR(pos.x, 5.0f, 1e-5f);
    EXPECT_NEAR(yaw, 45.0f, 1e-3f);
}

TEST(PlayerHistory, RefusesQueryOlderThanOldestSample) {
    PlayerHistory h;
    h.push(make(10.0, 0.0f));
    h.push(make(11.0, 1.0f));
    glm::vec3 pos;
    float yaw = 0;
    // 5.0 is way older than any sample we have — extrapolating
    // backwards would be garbage, refuse instead.
    EXPECT_FALSE(h.sampleAt(5.0, pos, yaw));
}

TEST(PlayerHistory, PinsToNewestWhenQueryIsInTheFuture) {
    PlayerHistory h;
    h.push(make(10.0, 1.0f));
    h.push(make(11.0, 2.0f));
    glm::vec3 pos;
    float yaw = 0;
    EXPECT_TRUE(h.sampleAt(20.0, pos, yaw));
    EXPECT_FLOAT_EQ(pos.x, 2.0f);
}

TEST(PlayerHistory, RingBufferKeepsLastNSamples) {
    // 128 ring entries; push 200 and verify we still find the recent ones.
    PlayerHistory h;
    for (int i = 0; i < 200; ++i) {
        h.push(make(static_cast<double>(i), static_cast<float>(i)));
    }
    glm::vec3 pos;
    float yaw = 0;
    EXPECT_TRUE(h.sampleAt(150.5, pos, yaw));
    EXPECT_NEAR(pos.x, 150.5f, 1e-3f);
    // Anything way older than the 128-window should be gone now.
    EXPECT_FALSE(h.sampleAt(10.0, pos, yaw));
}

TEST(LagCompensation, RewindCapRefusesAncientShots) {
    LagCompensation lc;
    HistorySample h;
    h.tServer = 0.0;
    h.pos     = glm::vec3(0.0f);
    lc.recordSample(1, h);

    // shooter=2 firing at tServerNow=1.0, claiming ping=10s → view-time
    // would be way past the 200ms cap.
    const auto hit =
        lc.raycast(2, glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), 10000, 1.0);
    EXPECT_FALSE(hit.has_value());
}

TEST(LagCompensation, RaycastHitsRewoundTarget) {
    LagCompensation lc;
    // Victim at x=2 at t=0.0
    HistorySample h;
    h.tServer = 0.0;
    h.pos     = glm::vec3(2.0f, 0.0f, 0.0f);
    lc.recordSample(1, h);
    // ...and at x=10 a tiny moment later.
    h.tServer = 0.05;
    h.pos     = glm::vec3(10.0f, 0.0f, 0.0f);
    lc.recordSample(1, h);

    // Shooter at origin fires +X at "now=0.05" with ping=0 — view-time
    // is 0.05 - 0.0 - 0.1 = -0.05, which the cap will... actually allow,
    // since |delta| = 0.1 == 0.2 cap. The history at -0.05 doesn't
    // exist, so the rewind refuses by sampleAt returning false.
    auto hit = lc.raycast(2, glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), 0, 0.05);
    EXPECT_FALSE(hit.has_value());

    // Now fire with view-time = 0.0 (close enough to the first sample)
    // — that's tServerNow=0.1, ping=0, viewTime = 0.1 - 0 - 0.1 = 0.0.
    hit = lc.raycast(2, glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), 0, 0.1);
    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->victim, 1u);
}

TEST(LagCompensation, ShooterCantHitSelf) {
    LagCompensation lc;
    HistorySample h;
    h.tServer = 0.0;
    h.pos     = glm::vec3(5.0f, 0.0f, 0.0f);
    lc.recordSample(7, h);

    auto hit = lc.raycast(7, glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), 0, 0.1);
    EXPECT_FALSE(hit.has_value());
}
