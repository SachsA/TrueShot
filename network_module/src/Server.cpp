#include <enet/enet.h>

#include "Network/Bitstream.h"
#include "Network/PacketTypes.h"
#include "Network/Server.h"
#include <algorithm>
#include <cmath>
#include <iostream>

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
        simulateTick();
        broadcastSnapshot();
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
        }
        queue.clear();
    }
}

void Server::applyInput(PlayerState& ps, const InputState& in) {
    // Clamp inputs hard. This is the seed of anti-cheat: the client
    // never gets to push us past these bounds, no matter what it sends.
    const float fwd = std::clamp(static_cast<float>(in.moveForward) / 127.0f, -1.0f, 1.0f);
    const float rgt = std::clamp(static_cast<float>(in.moveRight) / 127.0f, -1.0f, 1.0f);

    // Compute world-space move from yaw. Player faces +X for yaw=0;
    // forward = -Z when yaw=-90° (matches FPSCamera convention).
    const float yawRad = in.yaw * 3.1415926535f / 180.0f;
    const float cosY   = std::cos(yawRad);
    const float sinY   = std::sin(yawRad);

    ps.pos.x += (fwd * cosY + rgt * sinY) * kServerMoveSpeed * kFixedDt;
    ps.pos.z += (fwd * sinY - rgt * cosY) * kServerMoveSpeed * kFixedDt;

    ps.yaw        = std::clamp(in.yaw, -180.0f, 180.0f);
    ps.pitch      = std::clamp(in.pitch, -89.0f, 89.0f);
    ps.stateFlags = EntityFlag::Alive | EntityFlag::OnGround;
    if (in.buttons & InputButton::Crouch) ps.stateFlags |= EntityFlag::Crouching;
    if (in.buttons & InputButton::ADS) ps.stateFlags |= EntityFlag::Aiming;
    if (in.buttons & InputButton::Fire) ps.stateFlags |= EntityFlag::Firing;
    if (in.buttons & InputButton::Reload) ps.stateFlags |= EntityFlag::Reloading;

    ps.lastAckedSeq = std::max(ps.lastAckedSeq, in.seq);
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
    ENetPacket* pkt = enet_packet_create(bytes.data(), bytes.size(), ENET_PACKET_FLAG_UNSEQUENCED);
    if (!pkt) return;
    if (enet_peer_send(asPeer(peer), channel, pkt) == 0) {
        ++m_PacketsSent;
    } else {
        enet_packet_destroy(pkt);
    }
}

} // namespace Net
