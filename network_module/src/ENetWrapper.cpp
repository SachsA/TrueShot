        #include "Network/ENetWrapper.h"
#include <iostream>
namespace Net {

ENetContext::~ENetContext() { destroy(); }

bool ENetContext::createServer(uint16_t port, size_t maxClients) {
    if (host) destroy();
    enet_address_set_host(&address, "0.0.0.0");
    address.port = port;
    host = enet_host_create(&address, (enet_uint32)maxClients, 2, 0, 0);
    if (!host) { std::cerr<<"ENet server create failed"<<std::endl; return false; }
    return true;
}
bool ENetContext::createClient() {
    if (host) destroy();
    host = enet_host_create(nullptr, 1, 2, 0, 0);
    if (!host) { std::cerr<<"ENet client create failed"<<std::endl; return false; }
    return true;
}
void ENetContext::destroy() {
    if (host) {
        enet_host_destroy(host);
        host = nullptr;
    }
}
ENetPeer* ENetContext::connect(const std::string& hostName, uint16_t port, uint32_t timeout) {
    if (!host) return nullptr;
    ENetAddress addr;
    enet_address_set_host(&addr, hostName.c_str());
    addr.port = port;
    return enet_host_connect(host, &addr, 2, 0);
}
void ENetContext::service(std::function<void(ENetEvent&)> handler, uint32_t timeout_ms) {
    if (!host) return;
    ENetEvent event;
    while (enet_host_service(host, &event, timeout_ms) > 0) {
        handler(event);
    }
}

} // namespace Net
