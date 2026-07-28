#pragma once

#include "netcode/net_common.h"
#include "netcode/net_sim.h"

#include <array>
#include <cstddef>
#include <cstdint>

// ---------------------------------------------------------------------
// Client-side prediction + server reconciliation for the local player.
//
// Flow per simulation tick (128 Hz, lock-step with the server):
//
//   1. Gather InputState from the player (keys, mouse).
//   2. Push it into the pending ring + send it on the wire.
//   3. Apply it to the local SimState immediately (prediction) so that
//      WASD feels instant regardless of RTT.
//
// When a Snapshot arrives carrying ackSeq for the local player:
//
//   4. Drop every pending input with seq <= ackSeq.
//   5. Reset the local SimState to the server's authoritative pos/yaw.
//   6. Replay every remaining pending input from there to "catch up"
//      to the present.
//   7. Optionally smooth the visible position if the corrected state
//      drifted from where we had predicted (snap/lerp).
//
// See docs/adr/0002-netcode-architecture.md and ADR 0005 for design.
// ---------------------------------------------------------------------

namespace Net {

// One input we've sent but not yet seen ack'd. Stored so we can replay
// it on top of the corrected server state.
struct PendingInput {
    uint32_t seq = 0;
    InputState in{};
};

class ClientPrediction {
public:
    // The predicted "as of right now" state of the local player. The
    // renderer reads from here every frame.
    const SimState& state() const { return m_State; }

    // Push the latest input and advance the simulation by one tick.
    // Returns the seq stamped on the input (caller uses it for the
    // wire packet).
    void predict(const InputState& in);

    // Server has acknowledged everything up to `ackSeq` and the
    // authoritative state at that ack is `authoritative`. Drop pending
    // inputs, snap or lerp to the corrected state, replay any newer
    // pending inputs.
    void reconcile(uint32_t ackSeq, const SimState& authoritative);

    // How many inputs are currently in flight (sent but not ack'd).
    size_t pendingCount() const { return m_PendingCount; }

    // For the HUD: the magnitude of the last reconciliation correction.
    // 0 = perfectly predicted, large = mispredicting.
    float lastCorrectionMeters() const { return m_LastCorrection; }

private:
    // Ring buffer of pending inputs. At 128 Hz a 256-entry ring covers
    // 2 seconds of unacknowledged inputs — way more than any realistic
    // RTT. If we ever overflow this we're in serious trouble anyway.
    static constexpr size_t kRingSize = 256;
    std::array<PendingInput, kRingSize> m_Ring{};
    size_t m_RingHead     = 0; // next write slot
    size_t m_PendingCount = 0;

    SimState m_State{};
    float m_LastCorrection = 0.0f;

    // Helper: get the i-th oldest pending input (0 = oldest).
    PendingInput& atOldest(size_t i);
    const PendingInput& atOldest(size_t i) const;
};

} // namespace Net
