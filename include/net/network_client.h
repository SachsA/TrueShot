#pragma once

#include "Network/NetCommon.h"

#include <atomic>
#include <cstdint>
#include <deque>
#include <memory>
#include <string>

// Hide ENet behind opaque pointers so non-net translation units don't
// pay the include cost. The real types are resolved inside the .cpp.

// =====================================================================
// NetworkClient — owns the ENet socket on the game side.
//
// Responsibilities (Phase 1.4):
//   * Connect/disconnect to a TrueShot dedicated server.
//   * Send one ClientInput per server tick (caller-driven).
//   * Pop received Snapshots into an internal queue for the caller.
//   * Expose minimal metrics (RTT, packets, bytes) so the HUD/debug
//     can display them later (Phase 1.9).
//
// Out of scope here:
//   * Building the InputState from PlayerController — caller's job
//     (Application::run for now).
//   * Prediction / reconciliation — Phase 1.7.
//   * Interpolation of remote entities — Phase 1.6.
//
// The class is single-threaded. Calls to `tick()` drive both send
// and receive on the main game thread.
// =====================================================================
class NetworkClient {
public:
    enum class State : uint8_t {
        Disconnected = 0,
        Connecting,
        Connected,
        Failed,
    };

    NetworkClient();
    ~NetworkClient();

    NetworkClient(const NetworkClient&)            = delete;
    NetworkClient& operator=(const NetworkClient&) = delete;

    // Initialise ENet, create a client host. Returns false on init failure.
    // Safe to call multiple times — idempotent.
    bool initialize();

    // Tear down all sockets. Safe to call multiple times.
    void shutdown();

    // Begin connecting to the given host:port. Non-blocking — the actual
    // handshake happens through later `tick()` calls. Watch state() to
    // know when it's done.
    bool connectTo(const std::string& host, uint16_t port = Net::kDefaultPort);

    // Politely disconnect (sends an ENet disconnect; server sees it).
    void disconnect();

    // Drive one network step: service ENet events, send pending input
    // packets, pop received snapshots into m_RxSnapshots.
    // Call this once per game frame (NOT once per simulation tick).
    void tick();

    // Queue an InputState to be sent on the next tick(). The client is
    // expected to call this exactly once per simulation tick (128 Hz).
    void sendInput(const Net::InputState& input);

    // Pop the next received Snapshot, or return false if the queue is
    // empty. Caller is expected to drain this on every frame.
    bool popSnapshot(Net::Snapshot& outSnap);

    // ----- Metrics (read-only) -----
    State state() const { return m_State; }
    uint32_t roundTripMs() const { return m_RoundTripMs; }
    uint64_t packetsSent() const { return m_PacketsSent; }
    uint64_t packetsRecv() const { return m_PacketsRecv; }
    uint64_t bytesSent() const { return m_BytesSent; }
    uint64_t bytesRecv() const { return m_BytesRecv; }
    Net::PlayerId localId() const { return m_LocalPlayerId; }

private:
    void onPeerConnect();
    void onPeerDisconnect();
    void onPacket(const uint8_t* data, size_t len);

    // Handles individual packet types after the header is validated.
    void handleSnapshot(const uint8_t* body, size_t len);
    void handleHandshakeAck(const uint8_t* body, size_t len);

    bool m_EnetInitialized = false;
    // Opaque pointers to ENetHost / ENetPeer — the real types are
    // resolved inside the .cpp via reinterpret_cast.
    void* m_Host                  = nullptr;
    void* m_Peer                  = nullptr;
    State m_State                 = State::Disconnected;
    Net::PlayerId m_LocalPlayerId = 0;

    // Snapshots waiting to be consumed by the caller (FIFO).
    std::deque<Net::Snapshot> m_RxSnapshots;

    // Metrics
    uint32_t m_RoundTripMs = 0;
    uint64_t m_PacketsSent = 0;
    uint64_t m_PacketsRecv = 0;
    uint64_t m_BytesSent   = 0;
    uint64_t m_BytesRecv   = 0;
};
