#pragma once

#include <cstdint>

// ---------------------------------------------------------------------
// NetMetrics — a snapshot of every netcode number the HUD can display.
//
// Owned by `Application`, refreshed every frame from the live
// NetworkClient + ClientPrediction + RemotePlayerRegistry pointers, and
// rendered by `Hud::drawNetPanel`.
//
// The "live" counters (packets, bytes) are cumulative — we keep a
// previous-sample state inside `NetMetricsSampler` to derive
// bandwidth-per-second figures via an EMA. Snapshot rate is computed
// from a hit counter incremented by `noteSnapshotReceived`.
// ---------------------------------------------------------------------

namespace Net {

struct NetMetrics {
    // Connection state — 0=disconnected, 1=connecting, 2=connected, 3=failed.
    uint8_t state       = 0;

    uint32_t rttMs      = 0;
    uint32_t localId    = 0;
    uint32_t serverTick = 0; // last server tick we saw on a snapshot
    uint32_t localTick  = 0; // tick we're currently predicting at

    // Cumulative since connect.
    uint64_t packetsSent = 0;
    uint64_t packetsRecv = 0;
    uint64_t bytesSent   = 0;
    uint64_t bytesRecv   = 0;

    // Derived (EMA-smoothed over ~1 s).
    float bytesSentPerSec = 0.0f;
    float bytesRecvPerSec = 0.0f;
    float snapshotsPerSec = 0.0f;

    // Prediction health.
    uint32_t pendingInputs     = 0;
    float lastCorrectionMeters = 0.0f;
    uint32_t remotePlayerCount = 0;
};

// Samples the cumulative counters once per frame and computes
// per-second derivatives via a low-pass filter. Stateless from the
// caller's perspective beyond "feed me the live counters".
class NetMetricsSampler {
public:
    // Update metrics. `deltaTime` is real wall time elapsed since the
    // previous call (the same dt you pass to your game loop).
    void update(NetMetrics& m, uint64_t cumBytesSent, uint64_t cumBytesRecv, double deltaTime);

    // Call once per Snapshot received, before update() runs for this
    // frame. The sampler converts the count into a smoothed rate.
    void noteSnapshotReceived() { ++m_SnapshotsSinceLastUpdate; }

private:
    uint64_t m_PrevBytesSent            = 0;
    uint64_t m_PrevBytesRecv            = 0;
    uint32_t m_SnapshotsSinceLastUpdate = 0;
    bool m_PrimedCounters               = false;
};

} // namespace Net
