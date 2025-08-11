        #include "Network/NetCommon.h"
#include "Network/ENetWrapper.h"
#include "Network/Bitstream.h"
#include "Network/PacketTypes.h"
#include <iostream>
#include <deque>

using namespace Net;

class ClientCore {
public:
    ENetContext ctx;
    ENetPeer* serverPeer = nullptr;
    Tick localTick = 0;
    std::deque<InputState> pendingInputs;
    EntityState predicted;

    bool Start() {
        if (enet_initialize() != 0) { std::cerr<<"ENet init failed"<<std::endl; return false; }
        if(!ctx.createClient()) return false;
        return true;
    }
    bool Connect(const std::string& host, uint16_t port) {
        serverPeer = ctx.connect(host, port);
        if(!serverPeer) return false;
        ctx.service([&](ENetEvent& ev){ if(ev.type==ENET_EVENT_TYPE_CONNECT) std::cout<<"Connected to server"<<std::endl; }, 500);
        return true;
    }
    void TickOnce() {
        ctx.service([&](ENetEvent& ev){ onEvent(ev); }, 1);
        InputState in{}; in.tick = ++localTick; in.seq = (uint32_t)localTick; in.forward = 1.0f; in.right = 0.0f; in.yaw = 0; in.pitch = 0; in.jump = false; in.fire = false;
        applyInput(predicted, in);
        pendingInputs.push_back(in);
        sendInput(in);
    }
    void applyInput(EntityState &st, const InputState &in) {
        float speed = 5.0f; float dt = 1.0f/64.0f;
        st.pos.x += in.forward * speed * dt;
        st.pos.z += in.right * speed * dt;
        st.yaw = in.yaw; st.pitch = in.pitch;
    }
    void sendInput(const InputState &in) {
        if(!serverPeer) return;
        BitWriter bw; bw.writePOD((uint8_t)PacketType::ClientInput); bw.writePOD(in);
        ENetPacket* pkt = enet_packet_create(bw.buf.data(), bw.buf.size(), ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(serverPeer, 0, pkt);
    }
    void onEvent(ENetEvent& ev) {
        switch(ev.type) {
            case ENET_EVENT_TYPE_RECEIVE: {
                if(ev.packet->dataLength < 1) break;
                uint8_t t = ev.packet->data[0];
                if(t == (uint8_t)PacketType::Snapshot) {
                    BitReader br(ev.packet->data+1, ev.packet->dataLength-1);
                    Snapshot s; if(!br.readPOD(s.tick)) break;
                    uint32_t n; if(!br.readPOD(n)) break;
                    s.entities.resize(n);
                    for(uint32_t i=0;i<n;i++) {
                        br.readPOD(s.entities[i].id);
                        br.readPOD(s.entities[i].pos.x); br.readPOD(s.entities[i].pos.y); br.readPOD(s.entities[i].pos.z);
                        br.readPOD(s.entities[i].vel.x); br.readPOD(s.entities[i].vel.y); br.readPOD(s.entities[i].vel.z);
                        br.readPOD(s.entities[i].yaw); br.readPOD(s.entities[i].pitch);
                    }
                    for(auto &e : s.entities) {
                        predicted.pos = e.pos;
                        std::deque<InputState> newPending;
                        for(auto &pin : pendingInputs) {
                            if(pin.tick > s.tick) {
                                applyInput(predicted, pin);
                                newPending.push_back(pin);
                            }
                        }
                        pendingInputs.swap(newPending);
                    }
                }
                enet_packet_destroy(ev.packet);
                break;
            }
            default: break;
        }
    }
};

#ifdef TRUESHOT_CLIENT
int main(int argc, char** argv) {
    const char* host = "127.0.0.1";
    if(argc>1) host = argv[1];
    ClientCore c;
    if(!c.Start()) return 1;
    if(!c.Connect(host, 7777)) { std::cerr<<"Connect failed"<<std::endl; return 2; }
    for(int i=0;i<500;i++) { c.TickOnce(); enet_host_flush(c.ctx.host); enet_host_service(c.ctx.host, nullptr, 5); }
    return 0;
}
#endif
