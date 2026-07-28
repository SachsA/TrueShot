// ClientPrediction tests — the ring buffer + reconciliation logic
// that gives the local player zero-perceived input lag.

#include "net/client_prediction.h"
#include "netcode/net_sim.h"

#include <gtest/gtest.h>

using namespace Net;

namespace {

InputState makeInput(uint32_t seq, int8_t moveFwd = 0, float yaw = 0.0f) {
    InputState in{};
    in.seq         = seq;
    in.moveForward = moveFwd;
    in.yaw         = yaw;
    return in;
}

} // namespace

TEST(ClientPrediction, StartsEmpty) {
    ClientPrediction cp;
    EXPECT_EQ(cp.pendingCount(), 0u);
    EXPECT_FLOAT_EQ(cp.state().pos.x, 0.0f);
}

TEST(ClientPrediction, PredictAdvancesStateAndEnqueues) {
    ClientPrediction cp;
    cp.predict(makeInput(1, 127, 0.0f));
    EXPECT_EQ(cp.pendingCount(), 1u);
    EXPECT_GT(cp.state().pos.x, 0.0f);
}

TEST(ClientPrediction, ReconcileMatchingPredictionIsNoOp) {
    ClientPrediction cp;
    // Predict 10 ticks of forward motion.
    for (uint32_t i = 1; i <= 10; ++i) {
        cp.predict(makeInput(i, 127, 0.0f));
    }
    const glm::vec3 beforePos = cp.state().pos;

    // Server's truth matches our prediction (it ran the same sim).
    SimState authoritative;
    for (uint32_t i = 1; i <= 10; ++i) {
        stepSim(authoritative, makeInput(i, 127, 0.0f));
    }

    cp.reconcile(10, authoritative);

    EXPECT_EQ(cp.pendingCount(), 0u);
    EXPECT_NEAR(cp.state().pos.x, beforePos.x, 1e-5f);
    EXPECT_LT(cp.lastCorrectionMeters(), 0.001f);
}

TEST(ClientPrediction, ReconcileReplaysUnackedInputs) {
    ClientPrediction cp;
    // Predict 10 ticks; server has only ack'd up to seq=4.
    for (uint32_t i = 1; i <= 10; ++i) {
        cp.predict(makeInput(i, 127, 0.0f));
    }

    SimState authAt4;
    for (uint32_t i = 1; i <= 4; ++i) {
        stepSim(authAt4, makeInput(i, 127, 0.0f));
    }

    cp.reconcile(4, authAt4);

    // 6 inputs (seqs 5-10) still pending after reconcile dropped 1-4.
    EXPECT_EQ(cp.pendingCount(), 6u);
    // And the state should have caught up to "10 ticks worth" regardless.
    SimState expected;
    for (uint32_t i = 1; i <= 10; ++i) {
        stepSim(expected, makeInput(i, 127, 0.0f));
    }
    EXPECT_NEAR(cp.state().pos.x, expected.pos.x, 0.01f);
}

TEST(ClientPrediction, BigDriftTriggersSnap) {
    ClientPrediction cp;
    cp.predict(makeInput(1, 127, 0.0f)); // predict tiny step at +X

    SimState authoritative;
    authoritative.pos = glm::vec3(10.0f, 0.0f, 0.0f); // server says we're at x=10

    cp.reconcile(1, authoritative);

    // After reconcile, pending=0, state snapped to authoritative.
    EXPECT_EQ(cp.pendingCount(), 0u);
    EXPECT_NEAR(cp.state().pos.x, 10.0f, 0.01f);
    EXPECT_GT(cp.lastCorrectionMeters(), 0.5f); // we report the big delta
}

TEST(ClientPrediction, OverflowDropsOldestInputsSilently) {
    ClientPrediction cp;
    // 256 is the ring size — push 300, expect at most 256 pending.
    for (uint32_t i = 1; i <= 300; ++i) {
        cp.predict(makeInput(i, 127, 0.0f));
    }
    EXPECT_LE(cp.pendingCount(), 256u);
}
