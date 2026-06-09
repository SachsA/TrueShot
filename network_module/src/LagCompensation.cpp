#include "Network/LagCompensation.h"
#include "Network/NetSim.h" // for kInterpDelaySeconds-style constant proximity

#include <algorithm>
#include <cmath>

// We mirror RemotePlayer's kInterpDelaySeconds here without #including
// the client-side header (the server module must not depend on the
// game-side include tree). The value MUST stay in sync; the static
// assertion lives in tests once we add them in Phase 1.10.
namespace Net {
namespace {
constexpr double kClientInterpDelaySeconds = 0.100;
} // namespace

// ---------------------------------------------------------------------
// PlayerHistory
// ---------------------------------------------------------------------

void PlayerHistory::push(const HistorySample& s) {
    m_Ring[m_Head] = s;
    m_Head         = (m_Head + 1) % kRingSize;
    if (m_Count < kRingSize) ++m_Count;
}

const HistorySample& PlayerHistory::at(size_t reverseIndex) const {
    const size_t idx = (m_Head + kRingSize - 1 - reverseIndex) % kRingSize;
    return m_Ring[idx];
}

bool PlayerHistory::sampleAt(double tTarget, glm::vec3& outPos, float& outYaw) const {
    if (m_Count == 0) return false;

    // Walk newest -> oldest, find the pair enclosing tTarget.
    const HistorySample* newer = nullptr;
    const HistorySample* older = nullptr;
    for (size_t i = 0; i < m_Count; ++i) {
        const auto& s = at(i);
        if (s.tServer >= tTarget) {
            newer = &s;
        } else {
            older = &s;
            break;
        }
    }

    if (!older && !newer) return false;

    // tTarget is newer than anything we have — pin to the freshest.
    if (!older) {
        outPos = newer->pos;
        outYaw = newer->yaw;
        return true;
    }

    // tTarget is older than anything we have — refuse rather than
    // extrapolate backwards into garbage.
    if (!newer) {
        return false;
    }

    const double span = newer->tServer - older->tServer;
    float t           = 0.0f;
    if (span > 1e-6) {
        t = static_cast<float>((tTarget - older->tServer) / span);
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
    }
    outPos = glm::mix(older->pos, newer->pos, t);
    outYaw = older->yaw + (newer->yaw - older->yaw) * t;
    return true;
}

// ---------------------------------------------------------------------
// Free functions
// ---------------------------------------------------------------------

double computeViewTime(double tServerNow, uint16_t clientPingMs) {
    // RTT/2 is the one-way travel time from client to server. The
    // shooter saw the world at:
    //   T_view = T_now - RTT/2 - clientInterpDelay
    const double halfRtt = (static_cast<double>(clientPingMs) * 0.001) * 0.5;
    return tServerNow - halfRtt - kClientInterpDelaySeconds;
}

PlayerHitbox makeHitbox(const glm::vec3& playerPos) {
    // Matches the placeholder render cube in Renderer::drawRemotePlayers
    // (scale 0.8 x 1.8 x 0.4, centered on playerPos with the bottom at
    // the foot, top at the head). When real character models arrive in
    // Phase 4 we replace this with proper bone-driven hitboxes.
    constexpr float kHalfWidth = 0.40f;
    constexpr float kHalfDepth = 0.20f;
    constexpr float kHeight    = 1.80f;
    PlayerHitbox b;
    b.mins = glm::vec3(playerPos.x - kHalfWidth, playerPos.y, playerPos.z - kHalfDepth);
    b.maxs = glm::vec3(playerPos.x + kHalfWidth, playerPos.y + kHeight, playerPos.z + kHalfDepth);
    return b;
}

namespace {

// Ray vs AABB slab test. Returns true and writes `tHit` (the distance
// along `dir` where the ray first enters the box) if the ray hits.
bool rayVsAabb(const glm::vec3& origin, const glm::vec3& dir, const PlayerHitbox& box,
               float& tHit) {
    float tmin = 0.0f;
    float tmax = 1e9f;
    for (int axis = 0; axis < 3; ++axis) {
        const float o  = origin[axis];
        const float d  = dir[axis];
        const float lo = box.mins[axis];
        const float hi = box.maxs[axis];

        if (std::fabs(d) < 1e-6f) {
            // Ray parallel to the slab.
            if (o < lo || o > hi) return false;
        } else {
            float t1 = (lo - o) / d;
            float t2 = (hi - o) / d;
            if (t1 > t2) std::swap(t1, t2);
            if (t1 > tmin) tmin = t1;
            if (t2 < tmax) tmax = t2;
            if (tmin > tmax) return false;
        }
    }
    if (tmin < 0.0f) return false; // behind the shooter
    tHit = tmin;
    return true;
}

} // namespace

// ---------------------------------------------------------------------
// LagCompensation
// ---------------------------------------------------------------------

void LagCompensation::recordSample(PlayerId id, const HistorySample& s) {
    m_Histories[id].push(s);
}

void LagCompensation::forgetPlayer(PlayerId id) {
    m_Histories.erase(id);
}

std::optional<LagCompHit> LagCompensation::raycast(PlayerId shooter, const glm::vec3& origin,
                                                   const glm::vec3& dir, uint16_t clientPingMs,
                                                   double tServerNow) const {
    // 1. Where was the world, from the shooter's perspective?
    const double tView = computeViewTime(tServerNow, clientPingMs);

    // 2. Cap the rewind. We're willing to go back at most kRewindCap
    //    seconds; beyond that, anti-cheat refuses.
    if (tServerNow - tView > kRewindCapSeconds) {
        return std::nullopt;
    }

    std::optional<LagCompHit> closest;
    float closestT = 1e9f;

    for (const auto& [id, hist] : m_Histories) {
        if (id == shooter) continue; // no self-shooting

        glm::vec3 rewoundPos;
        float rewoundYaw = 0.0f;
        if (!hist.sampleAt(tView, rewoundPos, rewoundYaw)) {
            continue; // not enough history for this victim
        }

        const PlayerHitbox box = makeHitbox(rewoundPos);
        float tHit             = 0.0f;
        if (!rayVsAabb(origin, dir, box, tHit)) continue;

        if (tHit < closestT) {
            closestT = tHit;
            LagCompHit hit;
            hit.victim   = id;
            hit.hitPoint = origin + dir * tHit;
            hit.distance = tHit;
            closest      = hit;
        }
    }

    return closest;
}

} // namespace Net
