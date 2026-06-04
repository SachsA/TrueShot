#pragma once
#include <enet/enet.h>

#include <functional>
#include <memory>
#include <string>

namespace Net {

struct ENetContext {
    ENetHost* host = nullptr;
    ENetAddress address{};
    ~ENetContext();
    bool createServer(uint16_t port, size_t maxClients = 32);
    bool createClient();
    void destroy();
    ENetPeer* connect(const std::string& host, uint16_t port, uint32_t timeout = 5000);
    void service(std::function<void(ENetEvent&)> handler, uint32_t timeout_ms);
};

} // namespace Net
