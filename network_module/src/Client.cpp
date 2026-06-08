#include "Network/Bitstream.h"
#include "Network/ENetWrapper.h"
#include "Network/NetCommon.h"
#include "Network/PacketTypes.h"
#include <deque>
#include <iostream>
#include <string>

using namespace Net;

// ---------------------------------------------------------------------
// ClientCore — minimal "send an input, echo state" prototype.
// Used by `trueshot_client` for round-trip testing. The full
// prediction + reconciliation logic lives in NetworkClient (Phase 1.4).
// ---------------------------------------------------------------------
class ClientCore {
public:
    ENetContext ctx;
    ENetPeer* serverPeer = nullptr;
    Tick localTick       = 0;
    std::deque<InputState> pendingInputs;
    EntityState predicted;

    bool Start() {
        if (enet_initialize() != 0) {
            std::cerr << "[Client] ENet init failed\n";
            return false;
        }
        return ctx.createClient();
    }

    bool Connect(const std::string& host, uint16_t port) {
        serverPeer = ctx.connect(host, port);
        if (!serverPeer) return false;
        ctx.service(
            [&](ENetEvent& ev) {
                if (ev.type == ENET_EVENT_TYPE_CONNECT) {
                    std::cout << "[Client] Connected to server " << host << ':' << port << '\n';
                }
            },
            500);
        return true;
    }

    void TickOnce() {
        ctx.service([&](ENetEvent& ev) { onEvent(ev); }, 1);

        // Hard-coded "always run forward" input for the prototype.
        InputState in{};
        in.tick         = ++localTick;
        in.seq          = static_cast<uint32_t>(localTick);
        in.moveForward  = 127; // full forward
        in.moveRight    = 0;
        in.yaw          = 0.0f;
        in.pitch        = 0.0f;
        in.buttons      = 0;
        in.clientPingMs = 0;

        applyInput(predicted, in);
        pendingInputs.push_back(in);
        sendInput(in);
    }

    // Same physics the server runs (well, the placeholder version of it).
    // In Phase 1.7 this is replaced by a call into PlayerController.
    void applyInput(EntityState& st, const InputState& in) {
        constexpr float kSpeed = 5.0f;
        constexpr float kDt    = 1.0f / 128.0f;
        st.pos.x += static_cast<float>(in.moveForward) / 127.0f * kSpeed * kDt;
        st.pos.z += static_cast<float>(in.moveRight) / 127.0f * kSpeed * kDt;
        st.yaw   = in.yaw;
        st.pitch = in.pitch;
    }

    void sendInput(const InputState& in) {
        if (!serverPeer) return;
        BitWriter bw;
        bw.reserve(32);
        serialize(bw, in);
        ENetPacket* pkt =
            enet_packet_create(bw.buf.data(), bw.buf.size(), ENET_PACKET_FLAG_UNSEQUENCED);
        enet_peer_send(serverPeer, kChannelUnreliable, pkt);
    }

    void onEvent(ENetEvent& ev) {
        switch (ev.type) {
        case ENET_EVENT_TYPE_RECEIVE: {
            BitReader br(ev.packet->data, ev.packet->dataLength);
            PacketType type;
            if (readHeader(br, type) && type == PacketType::Snapshot) {
                Snapshot snap;
                if (deserializeBody(br, snap)) {
                    handleSnapshot(snap);
                }
            }
            enet_packet_destroy(ev.packet);
            break;
        }
        default:
            break;
        }
    }

    // Lightweight reconciliation: snap to server state, then replay
    // every unacked input on top. Full logic moves to NetworkClient
    // in Phase 1.7.
    void handleSnapshot(const Snapshot& snap) {
        if (snap.entities.empty()) return;

        predicted.pos = snap.entities.front().pos;

        std::deque<InputState> stillPending;
        for (const auto& pin : pendingInputs) {
            if (pin.tick > snap.tick) {
                applyInput(predicted, pin);
                stillPending.push_back(pin);
            }
        }
        pendingInputs.swap(stillPending);
    }
};

#ifdef TRUESHOT_CLIENT
int main(int argc, char** argv) {
    const char* host = "127.0.0.1";
    if (argc > 1) host = argv[1];

    ClientCore c;
    if (!c.Start()) return 1;
    if (!c.Connect(host, kDefaultPort)) {
        std::cerr << "[Client] Connect failed\n";
        return 2;
    }

    for (int i = 0; i < 500; ++i) {
        c.TickOnce();
        enet_host_flush(c.ctx.host);
        enet_host_service(c.ctx.host, nullptr, 5);
    }
    return 0;
}
#endif
