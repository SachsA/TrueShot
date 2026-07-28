#pragma once

#include <glm/glm.hpp>

#include "netcode/net_common.h"

#include <array>
#include <cstdint>
#include <unordered_map>

// ---------------------------------------------------------------------
// Remote players - entities replicated from the server.
//
// The client never simulates the position of a remote player. Instead
// it keeps a small history of snapshots (timestamped server ticks) and
// renders the entity at `now - kInterpDelay` by linearly interpolating
// between the two enclosing snapshots.
//
// With kInterpDelay = 100 ms at 128 Hz we always have at least 12-13
// snapshots in the buffer, so a single dropped packet is invisible.
//
// See docs/adr/0002-netcode-architecture.md and
// docs/adr/0004-snapshot-interpolation.md.
// ---------------------------------------------------------------------

namespace Net {

// One sampled snapshot for one entity. Reconstructed from the
// authoritative EntityState in the server Snapshot.
struct RemotePlayerSample {
    Tick tick        = 0;   // server tick this sample represents
    double timestamp = 0.0; // local wall time we received it
    glm::vec3 pos{0.0f};
    float yaw          = 0.0f;
    float pitch        = 0.0f;
    uint8_t stateFlags = 0;
};

// Interpolation delay - how far behind the most recent snapshot we
// render remote entities, in seconds. 100 ms is the standard FPS
// netcode trade-off: small enough that flicks feel responsive, big
// enough to absorb 1-2 packet losses.
constexpr double kInterpDelaySeconds = 0.100;

// Beyond this many seconds without fresh data we fall back to a single
// frozen pose rather than extrapolating into nonsense.
constexpr double kExtrapolationMaxSeconds = 0.100;

class RemotePlayer {
public:
    explicit RemotePlayer(PlayerId id) : m_Id(id) {}

    PlayerId id() const { return m_Id; }

    // Push a server-side sample for this player. `localTimestamp` is the
    // wall clock at the moment we received the packet, used as the
    // reference for the interpolation delay.
    void pushSample(Tick tick, double localTimestamp, const glm::vec3& pos, float yaw, float pitch,
                    uint8_t stateFlags);

    // Returns the position/yaw/pitch/flags that should be rendered at
    // wall time `renderTimeNow - kInterpDelaySeconds`. Returns false if
    // we have no samples yet or the entity is dead.
    bool sample(double renderTimeNow, glm::vec3& outPos, float& outYaw, float& outPitch,
                uint8_t& outFlags) const;

    bool hasSamples() const { return m_SampleCount > 0; }

    // Drop samples older than the interpolation window - keeps the ring
    // buffer compact. Called automatically inside pushSample().
    void prune(double renderTimeNow);

private:
    PlayerId m_Id = 0;

    // Ring buffer. 64 samples = 500 ms of history at 128 Hz, plenty for
    // the 100 ms interp delay + any reasonable jitter.
    static constexpr size_t kRingSize = 64;
    std::array<RemotePlayerSample, kRingSize> m_Ring{};
    size_t m_RingHead    = 0; // index of the next slot to write
    size_t m_SampleCount = 0; // total alive entries (<= kRingSize)

    // Helper: get the i-th newest sample (0 = newest).
    const RemotePlayerSample& at(size_t reverseIndex) const;
};

// Owns the set of RemotePlayer entities. The client wires this up to
// receive each Snapshot.
class RemotePlayerRegistry {
public:
    // Called for every Snapshot received. The localPlayerId is excluded
    // from the registry - the local player is rendered from prediction,
    // not from server snapshots.
    void ingestSnapshot(const Snapshot& snap, double localTimestamp, PlayerId localPlayerId);

    // Iterate over every replicated remote player (read-only).
    const std::unordered_map<PlayerId, RemotePlayer>& players() const { return m_Players; }

    // Forget every entity. Useful on disconnect.
    void clear() { m_Players.clear(); }

private:
    std::unordered_map<PlayerId, RemotePlayer> m_Players;
};

} // namespace Net
