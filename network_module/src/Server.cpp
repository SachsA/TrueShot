#include <enet/enet.h>

#include "Network/Bitstream.h"
#include "Network/NetSim.h"
#include "Network/PacketTypes.h"
#include "Network/Server.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>

using namespace Net;

namespace {

inline ENetHost* asHost(void* p) {
    return static_cast<ENetHost*>(p);
}
inline ENetPeer* asPeer(void* p) {
    return static_cast<ENetPeer*>(p);
}

// Process-wide ENet refcount, shared with NetworkClient (listen-server
// hosts both in the same process and we must not double-init/free).
// We keep our own copy here rather than linking against NetworkClient
// because the dedicated server binary doesn't pull NetworkClient in.
int g_EnetRefcount = 0;

bool enetRetain() {
    if (g_EnetRefcount == 0) {
        if (enet_initialize() != 0) return false;
    }
    ++g_EnetRefcount;
    return true;
}

void enetRelease() {
    if (g_EnetRefcount > 0) {
        --g_EnetRefcount;
        if (g_EnetRefcount == 0) enet_deinitialize();
    }
}

// Reference move speed used by the placeholder physics. Real physics
// lives in PlayerController on the client side; we apply a coarse
// approximation here to keep replicated motion vaguely correct.
constexpr float kServerMoveSpeed = 5.0f; // units/s
constexpr float kFixedDt         = 1.0f / 128.0f;

// Phase 1.10 network simulator helpers. Defined here so Server::step
// can call them from the deferred-packet drain without a forward
// declaration dance.

// One thread-local RNG. The simulator only runs on the server thread,
// so a thread_local instance avoids both locking and reseeding.
std::mt19937& netSimRng() {
    static thread_local std::mt19937 rng(std::random_device{}());
    return rng;
}

bool rollLossDrop(float probability) {
    if (probability <= 0.0f) return false;
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(netSimRng()) < probability;
}

double rollJitterSec(uint32_t jitterMs) {
    if (jitterMs == 0) return 0.0;
    std::uniform_int_distribution<int> dist(-static_cast<int>(jitterMs),
                                            static_cast<int>(jitterMs));
    return dist(netSimRng()) * 0.001;
}

void enetEnqueue(void* peer, const std::vector<uint8_t>& bytes, uint8_t channel,
                 uint64_t& packetsSent) {
    if (!peer || bytes.empty()) return;
    ENetPacket* pkt = enet_packet_create(bytes.data(), bytes.size(), ENET_PACKET_FLAG_UNSEQUENCED);
    if (!pkt) return;
    if (enet_peer_send(asPeer(peer), channel, pkt) == 0) {
        ++packetsSent;
    } else {
        enet_packet_destroy(pkt);
    }
}

} // namespace

namespace Net {

Server::Server() = default;

Server::~Server() {
    stop();
}

bool Server::start(uint16_t port, size_t maxClients) {
    if (m_Host) return true; // already running

    if (!enetRetain()) {
        std::cerr << "[Server] enet_initialize failed\n";
        return false;
    }
    m_EnetInitialized = true;

    ENetAddress addr{};
    enet_address_set_host(&addr, "0.0.0.0");
    addr.port = port;

    m_Host    = enet_host_create(&addr, maxClients, kNumChannels, 0, 0);
    if (!m_Host) {
        std::cerr << "[Server] enet_host_create on port " << port << " failed\n";
        stop();
        return false;
    }
    m_Port = port;
    std::cout << "[Server] Listening on UDP " << port << " (max " << maxClients << " clients)\n";
    return true;
}

void Server::stop() {
    if (m_Host) {
        enet_host_destroy(asHost(m_Host));
        m_Host = nullptr;
    }
    m_PeerToPlayer.clear();
    m_Players.clear();
    m_Pending.clear();
    m_Tick        = 0;
    m_Accumulator = 0.0;
    if (m_EnetInitialized) {
        enetRelease();
        m_EnetInitialized = false;
    }
}

void Server::step(double frameDeltaSeconds) {
    if (!m_Host) return;

    // 1) Drain incoming events for as long as we got any. We use a 0 ms
    //    poll — the caller decides the cadence.
    ENetEvent ev;
    while (enet_host_service(asHost(m_Host), &ev, 0) > 0) {
        switch (ev.type) {
        case ENET_EVENT_TYPE_CONNECT:
            onConnect(ev.peer);
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            onDisconnect(ev.peer);
            break;
        case ENET_EVENT_TYPE_RECEIVE:
            ++m_PacketsRecv;
            onPacket(ev.peer, ev.packet->data, ev.packet->dataLength);
            enet_packet_destroy(ev.packet);
            break;
        default:
            break;
        }
    }

    // 2) Run fixed-timestep simulation ticks.
    m_Accumulator += frameDeltaSeconds;
    // Safety cap — same as TickClock — to prevent runaway catch-up
    // after a stall.
    if (m_Accumulator > 0.25) m_Accumulator = 0.25;

    constexpr double kStep = static_cast<double>(kFixedDt);
    while (m_Accumulator >= kStep) {
        m_Accumulator -= kStep;
        m_ServerTimeSec += kStep;
        simulateTick();
        broadcastSnapshot();
    }

    // 3) Phase 1.10: release any deferred packets whose simulated delay
    //    has elapsed. We use a stable partition so order is preserved
    //    relative to enqueue order (within the same release time).
    if (!m_DeferredOut.empty()) {
        auto it =
            std::stable_partition(m_DeferredOut.begin(), m_DeferredOut.end(),
                                  [now = m_ServerTimeSec](const DeferredPacket& d) {
                                      return d.releaseAtServerSec > now; // keep = still pending
                                  });
        for (auto sendIt = it; sendIt != m_DeferredOut.end(); ++sendIt) {
            enetEnqueue(sendIt->peer, sendIt->bytes, sendIt->channel, m_PacketsSent);
        }
        m_DeferredOut.erase(it, m_DeferredOut.end());
    }
}

// ---------------------------------------------------------------------
// Event handlers
// ---------------------------------------------------------------------

void Server::onConnect(void* peer) {
    const PlayerId id    = m_NextPlayerId++;
    m_PeerToPlayer[peer] = id;
    asPeer(peer)->data   = reinterpret_cast<void*>(static_cast<uintptr_t>(id));

    PlayerState ps{};
    ps.id         = id;
    ps.stateFlags = EntityFlag::Alive | EntityFlag::OnGround;
    m_Players[id] = ps;

    std::cout << "[Server] Connect id=" << id << " (" << m_Players.size() << " players online)\n";
}

void Server::onDisconnect(void* peer) {
    const auto it = m_PeerToPlayer.find(peer);
    if (it == m_PeerToPlayer.end()) return;
    const PlayerId id = it->second;
    m_Players.erase(id);
    m_Pending.erase(id);
    m_LagComp.forgetPlayer(id);
    m_PeerToPlayer.erase(it);
    std::cout << "[Server] Disconnect id=" << id << " (" << m_Players.size() << " players left)\n";
}

void Server::onPacket(void* peer, const uint8_t* data, size_t len) {
    const auto it = m_PeerToPlayer.find(peer);
    if (it == m_PeerToPlayer.end()) return; // unknown peer — drop
    const PlayerId id = it->second;

    BitReader br(data, len);
    PacketType type;
    if (!readHeader(br, type)) return;

    switch (type) {
    case PacketType::ClientInput: {
        InputState in;
        if (!deserializeBody(br, in)) return;
        m_Pending[id].push_back(in);
        break;
    }
    case PacketType::Disconnect:
        // The peer will tear down via ENet's own event soon.
        break;
    default:
        // Phase 2+ will add handshake / ping handling here.
        break;
    }
}

// ---------------------------------------------------------------------
// Simulation
// ---------------------------------------------------------------------

void Server::simulateTick() {
    ++m_Tick;

    // Apply every input that arrived since the previous tick, in the
    // order the client sent them. A real production server would replay
    // them per-tick using the client's tick number; for Phase 1.5 we
    // accept any in the same frame and apply them sequentially.
    for (auto& [id, queue] : m_Pending) {
        auto playerIt = m_Players.find(id);
        if (playerIt == m_Players.end()) continue;
        PlayerState& ps = playerIt->second;

        std::sort(queue.begin(), queue.end(),
                  [](const InputState& a, const InputState& b) { return a.tick < b.tick; });

        for (const auto& in : queue) {
            applyInput(ps, in);

            // Phase 1.8: if this input carried a Fire press, resolve it
            // against the rewound world. We do this immediately after
            // applying the input so the shooter's own pose is canonical
            // when we read it for the ray origin.
            if (in.buttons & InputButton::Fire) {
                handleFire(id, in);
            }
        }
        queue.clear();
    }

    // Record one history sample per player per tick, AFTER all inputs
    // for this tick have been applied. This is what lag compensation
    // rewinds against.
    for (const auto& [id, ps] : m_Players) {
        HistorySample h;
        h.tick       = m_Tick;
        h.tServer    = m_ServerTimeSec;
        h.pos        = glm::vec3(ps.pos.x, ps.pos.y, ps.pos.z);
        h.yaw        = ps.yaw;
        h.pitch      = ps.pitch;
        h.stateFlags = ps.stateFlags;
        m_LagComp.recordSample(id, h);
    }
}

void Server::applyInput(PlayerState& ps, const InputState& in) {
    // The simulation step is **shared** with the client (Net::stepSim).
    // Identical formula = identical output bits = no constant
    // reconciliation snaps on the client. See docs/adr/0002 and the
    // Phase 1.7 client prediction comment in application.cpp.
    //
    // PlayerState carries our wire-friendly POD `Net::Vec3` (defined in
    // NetCommon.h) while NetSim works in glm::vec3. We copy field-by-
    // field so neither type needs to know about the other.
    SimState s;
    s.pos        = glm::vec3(ps.pos.x, ps.pos.y, ps.pos.z);
    s.yaw        = ps.yaw;
    s.pitch      = ps.pitch;
    s.stateFlags = ps.stateFlags;

    stepSim(s, in);

    ps.pos.x        = s.pos.x;
    ps.pos.y        = s.pos.y;
    ps.pos.z        = s.pos.z;
    ps.yaw          = s.yaw;
    ps.pitch        = s.pitch;
    ps.stateFlags   = s.stateFlags;
    ps.lastAckedSeq = std::max(ps.lastAckedSeq, in.seq);
}

void Server::handleFire(PlayerId shooterId, const InputState& in) {
    const auto it = m_Players.find(shooterId);
    if (it == m_Players.end()) return;
    const PlayerState& shooter = it->second;

    // Build the ray from the shooter's eye. The hitbox tops out at
    // y = pos.y + 1.8 m; the "eye" sits near the top of the cube.
    constexpr float kEyeHeight = 1.65f;
    const glm::vec3 origin(shooter.pos.x, shooter.pos.y + kEyeHeight, shooter.pos.z);

    // Direction from yaw/pitch (degrees). Same convention as FPSCamera:
    // yaw=0 looks down +X, yaw=-90 looks down -Z.
    const float yawRad   = in.yaw * 3.1415926535f / 180.0f;
    const float pitchRad = in.pitch * 3.1415926535f / 180.0f;
    const glm::vec3 dir(std::cos(pitchRad) * std::cos(yawRad), std::sin(pitchRad),
                        std::cos(pitchRad) * std::sin(yawRad));

    const auto hit = m_LagComp.raycast(shooterId, origin, dir, in.clientPingMs, m_ServerTimeSec);
    if (hit) {
        ++m_LagCompHits;
        std::cout << "[Server] LAG-COMP HIT shooter=" << shooterId << " victim=" << hit->victim
                  << " dist=" << hit->distance << "m"
                  << " ping=" << in.clientPingMs << "ms\n";
        // Phase 1.8 stops here — we only resolve the hit and log it.
        // Damage application, kill feed, score, and an Event packet
        // broadcasting the hit to all clients land in Phase 2 (match
        // structure + HP system).
    }
}

// ---------------------------------------------------------------------
// Snapshot broadcast
// ---------------------------------------------------------------------

void Server::broadcastSnapshot() {
    if (m_Players.empty()) return;

    // Build one Snapshot containing every alive player. Each peer gets
    // their personalised ackSeq stamped in — we serialise once per peer
    // because of that. Cheap enough for 10 entities.
    for (auto& [peer, id] : m_PeerToPlayer) {
        const auto it = m_Players.find(id);
        if (it == m_Players.end()) continue;

        Snapshot snap;
        snap.tick   = m_Tick;
        snap.ackSeq = it->second.lastAckedSeq;
        snap.entities.reserve(m_Players.size());
        for (const auto& [oid, ps] : m_Players) {
            EntityState e;
            e.id         = ps.id;
            e.pos        = ps.pos;
            e.yaw        = ps.yaw;
            e.pitch      = ps.pitch;
            e.stateFlags = ps.stateFlags;
            snap.entities.push_back(e);
        }

        BitWriter bw;
        bw.reserve(256);
        serialize(bw, snap);
        sendTo(peer, bw.buf, kChannelUnreliable);
    }
}

void Server::sendTo(void* peer, const std::vector<uint8_t>& bytes, uint8_t channel) {
    if (!peer || bytes.empty()) return;

    // Fast path: no simulation, hand straight to ENet.
    if (m_NetSim.lossProbability <= 0.0f && m_NetSim.baseDelayMs == 0 && m_NetSim.jitterMs == 0) {
        enetEnqueue(peer, bytes, channel, m_PacketsSent);
        return;
    }

    // Loss: drop the packet entirely.
    if (rollLossDrop(m_NetSim.lossProbability)) {
        ++m_NetSimDropped;
        return;
    }

    // Delay + jitter: queue for later release. We allow negative jitter
    // but clamp the resulting release time to "now" so we never send
    // packets before they were originally enqueued.
    const double baseSec   = m_NetSim.baseDelayMs * 0.001;
    const double jitterSec = rollJitterSec(m_NetSim.jitterMs);
    double release         = m_ServerTimeSec + baseSec + jitterSec;
    if (release < m_ServerTimeSec) release = m_ServerTimeSec;

    if (release <= m_ServerTimeSec) {
        // Effectively zero delay after jitter — pass through.
        enetEnqueue(peer, bytes, channel, m_PacketsSent);
        return;
    }

    DeferredPacket dp;
    dp.peer               = peer;
    dp.bytes              = bytes;
    dp.channel            = channel;
    dp.releaseAtServerSec = release;
    m_DeferredOut.push_back(std::move(dp));
}

} // namespace Net
