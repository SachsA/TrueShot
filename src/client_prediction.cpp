#include <glm/geometric.hpp>

#include "net/client_prediction.h"

#include <algorithm>

namespace Net {

// ---------------------------------------------------------------------
// Tuning knobs for the snap-vs-lerp threshold. Inlined here so the
// header doesn't carry policy.
// ---------------------------------------------------------------------

namespace {

// Below this, we don't bother correcting at all — it's floating-point
// noise from the angle-quantisation round trip.
constexpr float kIgnoreCorrectionMeters = 0.02f;

// Above this, the prediction is too wrong to smooth — snap.
constexpr float kSnapThresholdMeters = 0.50f;

} // namespace

PendingInput& ClientPrediction::atOldest(size_t i) {
    const size_t base = (m_RingHead + kRingSize - m_PendingCount) % kRingSize;
    return m_Ring[(base + i) % kRingSize];
}

const PendingInput& ClientPrediction::atOldest(size_t i) const {
    const size_t base = (m_RingHead + kRingSize - m_PendingCount) % kRingSize;
    return m_Ring[(base + i) % kRingSize];
}

void ClientPrediction::predict(const InputState& in) {
    // 1. Apply locally so the renderer sees the move this frame.
    stepSim(m_State, in);

    // 2. Record so we can replay it after reconciliation.
    PendingInput p;
    p.seq              = in.seq;
    p.in               = in;

    m_Ring[m_RingHead] = p;
    m_RingHead         = (m_RingHead + 1) % kRingSize;
    if (m_PendingCount < kRingSize) ++m_PendingCount;
    // If we *did* overflow, the oldest entry is now silently dropped —
    // that's the right behaviour: it's been in flight for 2 s without
    // an ack, the connection is effectively dead.
}

void ClientPrediction::reconcile(uint32_t ackSeq, const SimState& authoritative) {
    // 1. Drop every pending input with seq <= ackSeq.
    size_t dropped = 0;
    while (m_PendingCount > 0 && atOldest(0).seq <= ackSeq) {
        ++dropped;
        --m_PendingCount;
    }
    (void)dropped;

    // 2. Measure the mispredict — how far the server says we are from
    //    where we thought we were.
    const glm::vec3 delta = authoritative.pos - m_State.pos;
    const float distance  = glm::length(delta);
    m_LastCorrection      = distance;

    if (distance < kIgnoreCorrectionMeters && m_PendingCount == 0) {
        // We were right; nothing to do.
        m_State = authoritative;
        return;
    }

    // 3. Reset to the authoritative state and replay every still-pending
    //    input on top. After this loop, m_State is "where the server
    //    would tell us we are at our latest input seq".
    SimState replayState = authoritative;
    for (size_t i = 0; i < m_PendingCount; ++i) {
        stepSim(replayState, atOldest(i).in);
    }

    if (distance >= kSnapThresholdMeters) {
        // Snap — too far gone for a smooth correction.
        m_State = replayState;
        return;
    }

    // Soft correction: snap orientation (it's already what the player
    // controls so any smoothing here feels like input lag), nudge
    // position toward the corrected one by a small fraction. The next
    // few reconciliations will keep nudging until we're aligned.
    // Phase 1.7 uses 25 % per reconciliation; tune later if needed.
    constexpr float kLerpAlpha = 0.25f;
    m_State.pos                = m_State.pos + (replayState.pos - m_State.pos) * kLerpAlpha;
    m_State.yaw                = replayState.yaw;
    m_State.pitch              = replayState.pitch;
    m_State.stateFlags         = replayState.stateFlags;
}

} // namespace Net
