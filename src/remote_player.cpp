#include "net/remote_player.h"

#include "Network/PacketTypes.h"

#include <algorithm>

namespace Net {

// ---------------------------------------------------------------------
// RemotePlayer
// ---------------------------------------------------------------------

void RemotePlayer::pushSample(Tick tick, double localTimestamp, const glm::vec3& pos, float yaw,
                              float pitch, uint8_t stateFlags) {
    RemotePlayerSample s;
    s.tick       = tick;
    s.timestamp  = localTimestamp;
    s.pos        = pos;
    s.yaw        = yaw;
    s.pitch      = pitch;
    s.stateFlags = stateFlags;

    m_Ring[m_RingHead] = s;
    m_RingHead         = (m_RingHead + 1) % kRingSize;
    if (m_SampleCount < kRingSize) ++m_SampleCount;

    // Keep the window tight. Anything older than 4x the interp delay
    // is irrecoverable for our purposes.
    prune(localTimestamp);
}

const RemotePlayerSample& RemotePlayer::at(size_t reverseIndex) const {
    // reverseIndex 0 = newest, 1 = second-newest, ...
    const size_t idx = (m_RingHead + kRingSize - 1 - reverseIndex) % kRingSize;
    return m_Ring[idx];
}

void RemotePlayer::prune(double renderTimeNow) {
    // Walk from the oldest sample and forget anything past 4x the delay.
    constexpr double kKeepWindow = kInterpDelaySeconds * 4.0;
    while (m_SampleCount > 1) {
        const auto& oldest = at(m_SampleCount - 1);
        if (renderTimeNow - oldest.timestamp <= kKeepWindow) break;
        --m_SampleCount;
    }
}

bool RemotePlayer::sample(double renderTimeNow, glm::vec3& outPos, float& outYaw,
                          float& outPitch, uint8_t& outFlags) const {
    if (m_SampleCount == 0) return false;

    const double renderTime = renderTimeNow - kInterpDelaySeconds;

    // Walk from newest to oldest, find the pair (next, prev) such that
    // prev.ts <= renderTime <= next.ts.
    const RemotePlayerSample* newer = nullptr;
    const RemotePlayerSample* older = nullptr;
    for (size_t i = 0; i < m_SampleCount; ++i) {
        const auto& s = at(i);
        if (s.timestamp >= renderTime) {
            newer = &s;
        } else {
            older = &s;
            break;
        }
    }

    if (!newer && !older) return false;

    // No old sample yet — render at the newest available (extrapolation
    // forward would only make sense if we had a velocity; we don't here).
    if (!older) {
        outPos    = newer->pos;
        outYaw    = newer->yaw;
        outPitch  = newer->pitch;
        outFlags  = newer->stateFlags;
        return (outFlags & EntityFlag::Alive) != 0;
    }

    // We have an older sample but no newer one — we've fallen behind.
    // Extrapolate linearly forward up to kExtrapolationMaxSeconds, then
    // freeze.
    if (!newer) {
        const double dt = renderTime - older->timestamp;
        outPos          = older->pos;
        outYaw          = older->yaw;
        outPitch        = older->pitch;
        outFlags        = older->stateFlags;
        (void)dt; // No velocity yet — freezing is correct for Phase 1.6.
        return (outFlags & EntityFlag::Alive) != 0;
    }

    // Normal case: linear interpolation between two snapshots.
    const double span = newer->timestamp - older->timestamp;
    float t           = 0.0f;
    if (span > 1e-6) {
        t = static_cast<float>((renderTime - older->timestamp) / span);
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
    }
    outPos    = glm::mix(older->pos, newer->pos, t);
    outYaw    = older->yaw + (newer->yaw - older->yaw) * t;
    outPitch  = older->pitch + (newer->pitch - older->pitch) * t;
    outFlags  = newer->stateFlags; // flags don't interpolate
    return (outFlags & EntityFlag::Alive) != 0;
}

// ---------------------------------------------------------------------
// RemotePlayerRegistry
// ---------------------------------------------------------------------

void RemotePlayerRegistry::ingestSnapshot(const Snapshot& snap, double localTimestamp,
                                          PlayerId localPlayerId) {
    for (const auto& e : snap.entities) {
        if (e.id == localPlayerId) continue; // never replicate the local player

        auto it = m_Players.find(e.id);
        if (it == m_Players.end()) {
            it = m_Players.emplace(e.id, RemotePlayer(e.id)).first;
        }
        it->second.pushSample(snap.tick, localTimestamp,
                              glm::vec3(e.pos.x, e.pos.y, e.pos.z),
                              e.yaw, e.pitch, e.stateFlags);
    }
}

} // namespace Net
