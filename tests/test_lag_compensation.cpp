// View-time formula + hitbox geometry tests. These pin the math behind
// peeker's-advantage compensation; if the formula drifts, every hit
// registers slightly wrong on every player at non-zero ping.

#include "netcode/lag_compensation.h"

#include <gtest/gtest.h>

using namespace Net;

TEST(ComputeViewTime, ZeroPingZeroInterp) {
    // With zero ping and the standard 100 ms interp delay, view time
    // is exactly 100 ms in the past.
    const double v = computeViewTime(1.000, 0);
    EXPECT_NEAR(v, 0.900, 1e-9);
}

TEST(ComputeViewTime, AddsHalfRtt) {
    // 100 ms ping → 50 ms half-RTT, total rewind = 150 ms.
    const double v = computeViewTime(1.000, 100);
    EXPECT_NEAR(v, 0.850, 1e-9);
}

TEST(ComputeViewTime, NeverProducesFutureTime) {
    // Even with the wildest fake-ping report, view-time stays in the
    // past (negative would be a logic bug).
    const double v = computeViewTime(1.000, 65535);
    EXPECT_LT(v, 1.000);
}

TEST(MakeHitbox, MatchesRenderCubeDimensions) {
    const glm::vec3 footPos(1.0f, 2.0f, 3.0f);
    const auto box = makeHitbox(footPos);

    // Width = 2 * halfWidth = 0.80; Depth = 2 * halfDepth = 0.40
    EXPECT_NEAR(box.maxs.x - box.mins.x, 0.80f, 1e-5f);
    EXPECT_NEAR(box.maxs.z - box.mins.z, 0.40f, 1e-5f);
    // Height = 1.80, mins.y = pos.y, maxs.y = pos.y + 1.80
    EXPECT_FLOAT_EQ(box.mins.y, 2.0f);
    EXPECT_NEAR(box.maxs.y - box.mins.y, 1.80f, 1e-5f);
}

TEST(MakeHitbox, CentersOnXZ) {
    const glm::vec3 footPos(1.0f, 0.0f, 5.0f);
    const auto box = makeHitbox(footPos);
    EXPECT_NEAR((box.mins.x + box.maxs.x) * 0.5f, 1.0f, 1e-5f);
    EXPECT_NEAR((box.mins.z + box.maxs.z) * 0.5f, 5.0f, 1e-5f);
}
