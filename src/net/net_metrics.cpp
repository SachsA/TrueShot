#include "net/net_metrics.h"

namespace Net {

// Low-pass filter coefficient for the per-second metrics. 0.20 means
// the displayed value catches up to a step change in ~5 frames at 60 fps
// (≈ 80 ms). That's snappy enough to feel "live" without being so
// jittery that the numbers strobe.
namespace {
constexpr float kEmaAlpha = 0.20f;

float emaUpdate(float prev, float current) {
    return prev + kEmaAlpha * (current - prev);
}
} // namespace

void NetMetricsSampler::update(NetMetrics& m, uint64_t cumBytesSent, uint64_t cumBytesRecv,
                               double deltaTime) {
    if (!m_PrimedCounters) {
        // First sample: no per-second derivative yet, just seed.
        m_PrevBytesSent            = cumBytesSent;
        m_PrevBytesRecv            = cumBytesRecv;
        m_PrimedCounters           = true;
        m_SnapshotsSinceLastUpdate = 0;
        return;
    }

    if (deltaTime > 1e-4) {
        const double instSent  = static_cast<double>(cumBytesSent - m_PrevBytesSent) / deltaTime;
        const double instRecv  = static_cast<double>(cumBytesRecv - m_PrevBytesRecv) / deltaTime;
        const double instSnaps = static_cast<double>(m_SnapshotsSinceLastUpdate) / deltaTime;

        m.bytesSentPerSec      = emaUpdate(m.bytesSentPerSec, static_cast<float>(instSent));
        m.bytesRecvPerSec      = emaUpdate(m.bytesRecvPerSec, static_cast<float>(instRecv));
        m.snapshotsPerSec      = emaUpdate(m.snapshotsPerSec, static_cast<float>(instSnaps));
    }

    m_PrevBytesSent            = cumBytesSent;
    m_PrevBytesRecv            = cumBytesRecv;
    m_SnapshotsSinceLastUpdate = 0;
}

} // namespace Net
