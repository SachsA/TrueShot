        #include "Network/NetCommon.h"
#include "Network/ENetWrapper.h"
#include "Network/Bitstream.h"
#include "Network/PacketTypes.h"
#include <unordered_map>
#include <iostream>

using namespace Net;

class ServerCore {
public:
    ENetContext ctx;
    uint16_t port = 7777;
    Tick serverTick = 0;
    std::unordered_map<ENetPeer*, PlayerId> peersToId;
    PlayerId nextPlayerId = 1;

    bool Start() {
        if (enet_initialize() != 0) { std::cerr<<"ENet init failed"<<std::endl; return false; }
        if (!ctx.createServer(port)) return false;
        std::cout<<"Server started on port "<<port<<std::endl;
        return true;
    }
    void TickOnce(uint32_t timeout_ms=1) {
        ctx.service([&](ENetEvent& ev){ onEvent(ev); }, timeout_ms);
    }
    void onEvent(ENetEvent& ev) {
        switch(ev.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                auto id = nextPlayerId++;
                peersToId[ev.peer] = id;
                ev.peer->data = (void*)(uintptr_t)id;
                std::cout<<"Client connected id="<<id<<std::endl;
                break;
            }
            case ENET_EVENT_TYPE_RECEIVE: {
                if(ev.packet->dataLength < 1) break;
                uint8_t t = ev.packet->data[0];
                if(t == (uint8_t)PacketType::ClientInput) {
                    BitReader br(ev.packet->data+1, ev.packet->dataLength-1);
                    InputState in{};
                    if(br.readPOD(in)) {
                        Snapshot s; s.tick = in.tick;
                        EntityState e; e.id = peersToId[ev.peer];
                        e.pos = {in.forward*5.0f, 0.0f, in.right*5.0f};
                        e.vel = {0,0,0};
                        e.yaw = in.yaw; e.pitch = in.pitch;
                        s.entities.push_back(e);
                        sendSnapshot(ev.peer, s);
                    }
                }
                enet_packet_destroy(ev.packet);
                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT: {
                std::cout<<"Client disconnected"<<std::endl;
                peersToId.erase(ev.peer);
                ev.peer->data = nullptr;
                break;
            }
            default: break;
        }
    }
    void sendSnapshot(ENetPeer* peer, const Snapshot& s) {
        BitWriter bw;
        bw.writePOD((uint8_t)PacketType::Snapshot);
        bw.writePOD(s.tick);
        uint32_t n = (uint32_t)s.entities.size();
        bw.writePOD(n);
        for(auto &e: s.entities) {
            bw.writePOD(e.id);
            bw.writePOD(e.pos.x); bw.writePOD(e.pos.y); bw.writePOD(e.pos.z);
            bw.writePOD(e.vel.x); bw.writePOD(e.vel.y); bw.writePOD(e.vel.z);
            bw.writePOD(e.yaw); bw.writePOD(e.pitch);
        }
        ENetPacket* pkt = enet_packet_create(bw.buf.data(), bw.buf.size(), ENET_PACKET_FLAG_UNSEQUENCED);
        enet_peer_send(peer, 0, pkt);
    }
};

#ifdef TRUESHOT_SERVER
int main() {
    ServerCore s;
    if(!s.Start()) return 1;
    while(true) {
        s.TickOnce(5);
    }
    return 0;
}
#endif
