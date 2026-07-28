#pragma once

#include <glm/glm.hpp>

#include "netcode/net_common.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>

// ---------------------------------------------------------------------
// Lag compensation — server-side hit registration that rewinds the
// world to where the shooter actually saw their target.
//
// The shooter sees remote players at:
//
//     T_view = T_now_server - RTT/2 - interpDelay
//            = T_now_server - (clientPingMs / 2) - 100ms
//
// At fire-time, the server rewinds every potential target to T_view
// and raycasts against THOSE positions. This is what makes "peeker's
// advantage" tolerable instead of impossible to play around.
//
// We cap the rewind window at 200 ms. Beyond that we treat the client
// as too laggy to be authoritative about hit registration (a common
// cheat is to spike ping deliberately to extend the rewind window).
//
// See docs/adr/0006-lag-compensation.md (Phase 1.8).
// ---------------------------------------------------------------------

namespace Net {

// Per-player position snapshot at a known server tick.
struct HistorySample {
    Tick tick      = 0;
    double tServer = 0.0; // server wall time when this state was canonical
    glm::vec3 pos{0.0f};
    float yaw          = 0.0f;
    float pitch        = 0.0f;
    uint8_t stateFlags = 0;
};

// One ring buffer of position samples per player. 128 samples at
// 128 Hz = 1 second of history — way past the 200 ms rewind cap.
class PlayerHistory {
public:
    void push(const HistorySample& s);

    // Resolve the player's position at server wall time `tTarget` by
    // linearly interpolating between the two enclosing samples.
    // Returns false if we don't have enough history (start of session)
    // or if tTarget is older than our oldest sample.
    bool sampleAt(double tTarget, glm::vec3& outPos, float& outYaw) const;

    bool empty() const { return m_Count == 0; }

private:
    static constexpr size_t kRingSize = 128;
    std::array<HistorySample, kRingSize> m_Ring{};
    size_t m_Head  = 0;
    size_t m_Count = 0;

    const HistorySample& at(size_t reverseIndex) const;
};

// Hard cap on how far we'll rewind. Beyond 200 ms we refuse — the
// connection is too lossy/slow to give the shooter a fair window, and
// extending it any further is a known cheat vector ("lag for free
// shots").
constexpr double kRewindCapSeconds = 0.200;

// What the shooter's screen looked like at fire-time, expressed as a
// server wall time. Computed from `clientPingMs` and the snapshot
// interpolation delay.
double computeViewTime(double tServerNow, uint16_t clientPingMs);

// Simple AABB representing a player's hurt-box at a given pose. Phase
// 1.8 uses a single box approximating the placeholder cube render —
// real hitboxes (head, torso, legs) land in Phase 4 with the real
// character model.
struct PlayerHitbox {
    glm::vec3 mins;
    glm::vec3 maxs;
};

PlayerHitbox makeHitbox(const glm::vec3& playerPos);

// Result of a server-side raycast against a rewound world.
struct LagCompHit {
    PlayerId victim    = 0;
    glm::vec3 hitPoint = glm::vec3(0.0f);
    float distance     = 0.0f;
};

// History container keyed by PlayerId. Owned by `Net::Server`.
class LagCompensation {
public:
    // Record the canonical state of one player at `tServer`. Called
    // once per simulation tick, *after* `stepSim`.
    void recordSample(PlayerId id, const HistorySample& s);

    // Resolve a shot. `shooter` is the id of the firing player (we
    // skip self-collision). `origin`/`dir` is the ray from the
    // shooter's eye at fire-time. `clientPingMs` is what the shooter
    // reported in the InputState that carried the Fire button.
    // `tServerNow` is the server's wall time when the input arrived.
    // Returns the closest hit player, if any.
    std::optional<LagCompHit> raycast(PlayerId shooter, const glm::vec3& origin,
                                      const glm::vec3& dir, uint16_t clientPingMs,
                                      double tServerNow) const;

    void forgetPlayer(PlayerId id);

private:
    std::unordered_map<PlayerId, PlayerHistory> m_Histories;
};

} // namespace Net
