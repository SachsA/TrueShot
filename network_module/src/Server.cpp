#include "Network/Bitstream.h"
#include "Network/ENetWrapper.h"
#include "Network/NetCommon.h"
#include "Network/PacketTypes.h"
#include <iostream>
#include <unordered_map>

using namespace Net;

// ---------------------------------------------------------------------
// ServerCore — minimal echo-style server prototype.
// This is the development scaffolding used by `trueshot_server`. The
// production tick + state replication will be written in Phase 1.5
// inside a proper class that GameWorld + lag comp can plug into.
// ---------------------------------------------------------------------
class ServerCore {
public:
    ENetContext ctx;
    uint16_t port   = kDefaultPort;
    Tick serverTick = 0;
    std::unordered_map<ENetPeer*, PlayerId> peersToId;
    PlayerId nextPlayerId = 1;

    bool Start() {
        if (enet_initialize() != 0) {
            std::cerr << "[Server] ENet init failed\n";
            return false;
        }
        if (!ctx.createServer(port)) return false;
        std::cout << "[Server] Listening on port " << port << '\n';
        return true;
    }

    void TickOnce(uint32_t timeoutMs = 1) {
        ctx.service([&](ENetEvent& ev) { onEvent(ev); }, timeoutMs);
        ++serverTick;
    }

    void onEvent(ENetEvent& ev) {
        switch (ev.type) {
        case ENET_EVENT_TYPE_CONNECT: {
            const PlayerId id  = nextPlayerId++;
            peersToId[ev.peer] = id;
            ev.peer->data      = reinterpret_cast<void*>(static_cast<uintptr_t>(id));
            std::cout << "[Server] Client connected id=" << id << '\n';
            break;
        }
        case ENET_EVENT_TYPE_RECEIVE: {
            handlePacket(ev.peer, ev.packet->data, ev.packet->dataLength);
            enet_packet_destroy(ev.packet);
            break;
        }
        case ENET_EVENT_TYPE_DISCONNECT: {
            const auto it = peersToId.find(ev.peer);
            if (it != peersToId.end()) {
                std::cout << "[Server] Client disconnected id=" << it->second << '\n';
                peersToId.erase(it);
            }
            ev.peer->data = nullptr;
            break;
        }
        default:
            break;
        }
    }

    // Decode one incoming packet and dispatch by type.
    void handlePacket(ENetPeer* peer, const uint8_t* data, size_t len) {
        BitReader br(data, len);
        PacketType type;
        if (!readHeader(br, type)) return;

        switch (type) {
        case PacketType::ClientInput: {
            InputState in{};
            if (!deserializeBody(br, in)) return;

            // Echo a placeholder Snapshot back. Real authoritative
            // simulation will land in Phase 1.5.
            Snapshot snap;
            snap.tick   = in.tick;
            snap.ackSeq = in.seq;
            snap.entities.resize(1);
            snap.entities[0].id         = peersToId[peer];
            snap.entities[0].pos.x      = static_cast<float>(in.moveForward) * 5.0f / 127.0f;
            snap.entities[0].pos.z      = static_cast<float>(in.moveRight) * 5.0f / 127.0f;
            snap.entities[0].yaw        = in.yaw;
            snap.entities[0].pitch      = in.pitch;
            snap.entities[0].stateFlags = EntityFlag::Alive | EntityFlag::OnGround;
            sendSnapshot(peer, snap);
            break;
        }
        default:
            // Unknown / unsupported here. Future phases will add Ping,
            // Handshake, Disconnect handling.
            break;
        }
    }

    void sendSnapshot(ENetPeer* peer, const Snapshot& snap) {
        BitWriter bw;
        bw.reserve(256);
        serialize(bw, snap);
        ENetPacket* pkt =
            enet_packet_create(bw.buf.data(), bw.buf.size(), ENET_PACKET_FLAG_UNSEQUENCED);
        enet_peer_send(peer, kChannelUnreliable, pkt);
    }
};

#ifdef TRUESHOT_SERVER
int main() {
    ServerCore s;
    if (!s.Start()) return 1;
    while (true) {
        s.TickOnce(5);
    }
}
#endif
