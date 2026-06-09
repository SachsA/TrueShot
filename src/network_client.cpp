#include <enet/enet.h>

#include "Network/Bitstream.h"
#include "Network/PacketTypes.h"
#include "net/network_client.h"

#include <cstring>
#include <iostream>

using namespace Net;

namespace {

// Helpers to recover the real ENet types from the opaque void* members.
// Named `asHost` / `asPeer` rather than `host` / `peer` to avoid
// clashing with the `host` parameter name in connectTo().
inline ENetHost* asHost(void* p) {
    return static_cast<ENetHost*>(p);
}
inline ENetPeer* asPeer(void* p) {
    return static_cast<ENetPeer*>(p);
}

// One process-wide refcount on enet_initialize / enet_deinitialize, so
// listen-server (where both a NetworkClient and a Server live in the
// same process) doesn't double-init or prematurely tear down ENet.
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
        if (g_EnetRefcount == 0) {
            enet_deinitialize();
        }
    }
}

} // namespace

NetworkClient::NetworkClient() = default;
NetworkClient::~NetworkClient() {
    shutdown();
}

bool NetworkClient::initialize() {
    if (m_EnetInitialized) return true;

    if (!enetRetain()) {
        std::cerr << "[Net] enet_initialize failed\n";
        return false;
    }
    m_EnetInitialized = true;

    // 1 outgoing connection (the server), 2 channels (reliable + unreliable),
    // unlimited bandwidth — we'll add per-client throttling in Phase 8.
    m_Host = enet_host_create(nullptr, 1, kNumChannels, 0, 0);
    if (!m_Host) {
        std::cerr << "[Net] enet_host_create (client) failed\n";
        shutdown();
        return false;
    }
    return true;
}

void NetworkClient::shutdown() {
    disconnect();
    if (m_Host) {
        enet_host_destroy(asHost(m_Host));
        m_Host = nullptr;
    }
    if (m_EnetInitialized) {
        enetRelease();
        m_EnetInitialized = false;
    }
    m_State = State::Disconnected;
}

bool NetworkClient::connectTo(const std::string& host, uint16_t port) {
    if (!m_Host) {
        std::cerr << "[Net] connectTo() called before initialize()\n";
        return false;
    }
    if (m_Peer) {
        std::cerr << "[Net] already connected/connecting — disconnect first\n";
        return false;
    }

    ENetAddress addr{};
    if (enet_address_set_host(&addr, host.c_str()) != 0) {
        std::cerr << "[Net] enet_address_set_host('" << host << "') failed\n";
        return false;
    }
    addr.port = port;

    m_Peer    = enet_host_connect(asHost(m_Host), &addr, kNumChannels, 0);
    if (!m_Peer) {
        std::cerr << "[Net] enet_host_connect failed (no free peers)\n";
        return false;
    }

    m_State = State::Connecting;
    std::cout << "[Net] Connecting to " << host << ':' << port << "...\n";
    return true;
}

void NetworkClient::disconnect() {
    if (!m_Peer) return;
    enet_peer_disconnect(asPeer(m_Peer), 0);
    // Give ENet ~50 ms to flush the disconnect message politely.
    if (m_Host) {
        ENetEvent ev;
        const uint32_t deadlineMs = 50;
        while (enet_host_service(asHost(m_Host), &ev, deadlineMs) > 0) {
            if (ev.type == ENET_EVENT_TYPE_DISCONNECT) break;
            if (ev.type == ENET_EVENT_TYPE_RECEIVE) enet_packet_destroy(ev.packet);
        }
    }
    m_Peer  = nullptr;
    m_State = State::Disconnected;
}

void NetworkClient::sendInput(const Net::InputState& input) {
    if (m_State != State::Connected || !m_Peer) return;

    BitWriter bw;
    bw.reserve(32);
    Net::serialize(bw, input);

    ENetPacket* pkt =
        enet_packet_create(bw.buf.data(), bw.buf.size(), ENET_PACKET_FLAG_UNSEQUENCED);
    if (!pkt) return;

    if (enet_peer_send(asPeer(m_Peer), kChannelUnreliable, pkt) == 0) {
        ++m_PacketsSent;
        m_BytesSent += bw.buf.size();
    } else {
        enet_packet_destroy(pkt);
    }
}

void NetworkClient::tick() {
    if (!m_Host) return;

    ENetEvent ev;
    while (enet_host_service(asHost(m_Host), &ev, 0) > 0) {
        switch (ev.type) {
        case ENET_EVENT_TYPE_CONNECT:
            onPeerConnect();
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            onPeerDisconnect();
            break;
        case ENET_EVENT_TYPE_RECEIVE:
            onPacket(ev.packet->data, ev.packet->dataLength);
            enet_packet_destroy(ev.packet);
            break;
        default:
            break;
        }
    }

    // Pull the current RTT measurement from ENet.
    if (m_Peer) {
        m_RoundTripMs = asPeer(m_Peer)->roundTripTime;
    }
}

bool NetworkClient::popSnapshot(Net::Snapshot& outSnap) {
    if (m_RxSnapshots.empty()) return false;
    outSnap = std::move(m_RxSnapshots.front());
    m_RxSnapshots.pop_front();
    return true;
}

// ---------------------------------------------------------------------
// Private — event handlers.
// ---------------------------------------------------------------------

void NetworkClient::onPeerConnect() {
    m_State = State::Connected;
    std::cout << "[Net] Connection established (peer up)\n";
    // Future: send Handshake packet here. For Phase 1.4 we leave the
    // server to assume the connecting peer is a valid client.
}

void NetworkClient::onPeerDisconnect() {
    std::cout << "[Net] Disconnected from server\n";
    m_Peer  = nullptr;
    m_State = State::Disconnected;
    m_RxSnapshots.clear();
}

void NetworkClient::onPacket(const uint8_t* data, size_t len) {
    ++m_PacketsRecv;
    m_BytesRecv += len;

    BitReader br(data, len);
    PacketType type;
    if (!readHeader(br, type)) {
        // Bad protocol or short packet — drop silently. Future phases
        // can add a metric for it (m_BadPackets++).
        return;
    }

    switch (type) {
    case PacketType::Snapshot:
        handleSnapshot(br.p, br.remaining);
        break;
    case PacketType::HandshakeAck:
        handleHandshakeAck(br.p, br.remaining);
        break;
    case PacketType::Disconnect:
        // The peer hung up — ENet will fire EVENT_TYPE_DISCONNECT on the
        // next service() loop, so just no-op here.
        break;
    default:
        // Unknown packet type — ignore. Will be useful when we add
        // optional packet types without breaking old clients.
        break;
    }
}

void NetworkClient::handleSnapshot(const uint8_t* body, size_t len) {
    BitReader br(body, len);
    Snapshot snap;
    if (!deserializeBody(br, snap)) {
        // Malformed snapshot — drop.
        return;
    }
    m_RxSnapshots.push_back(std::move(snap));
}

void NetworkClient::handleHandshakeAck(const uint8_t* body, size_t len) {
    BitReader br(body, len);
    HandshakeAck ack;
    if (!deserializeBody(br, ack)) return;
    if (ack.accepted) {
        m_LocalPlayerId = ack.slot;
        std::cout << "[Net] Handshake accepted, slot=" << ack.slot << '\n';
    } else {
        std::cerr << "[Net] Handshake rejected by server\n";
        disconnect();
    }
}
