#pragma once

#include "netcode/lag_compensation.h"
#include "netcode/net_common.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// Hide ENet behind opaque pointers — same trick as NetworkClient — so
// translation units that just want to spin up a Server don't pay the
// `enet.h` include cost.

namespace Net {

// =====================================================================
// Server — authoritative simulation host.
//
// Responsibilities (Phase 1.5):
//   * Accept ENet connections on a UDP port.
//   * Allocate a PlayerId slot per peer.
//   * Receive ClientInputs, apply them in the player's frame, clamp
//     anything out of bounds (anti-cheat foundation, Phase 9 builds
//     on this contract).
//   * Run a fixed 128 Hz simulation tick.
//   * Broadcast a full Snapshot of the world to every peer at that
//     same rate.
//
// Not in scope here (later phases):
//   * Lag compensation history (Phase 1.8).
//   * Delta-compressed snapshots (Phase 2 optimisation).
//   * Real physics — for now we apply move input * speed * dt and
//     keep the player on the ground.
//   * Handshake validation (Phase 2).
//
// Threading: single-threaded. The caller drives the server by calling
// `start()` once and `step(dt)` repeatedly from the main loop (or from
// a dedicated thread for the standalone `trueshot_server` binary).
// =====================================================================
class Server {
public:
    // Per-connected-player canonical state. The server is the only owner
    // of these — clients never see them directly, only as serialised
    // EntityStates inside Snapshots.
    struct PlayerState {
        PlayerId id = 0;
        Vec3 pos{};
        Vec3 vel{};
        float yaw          = 0.0f;
        float pitch        = 0.0f;
        uint8_t stateFlags = 0;
        // The last ClientInput seq we successfully applied. Echoed back
        // in Snapshot.ackSeq so the client knows what to drop from its
        // pending-input buffer (used in Phase 1.7 reconciliation).
        uint32_t lastAckedSeq = 0;
    };

    Server();
    ~Server();

    Server(const Server&)            = delete;
    Server& operator=(const Server&) = delete;

    // Simulated bad-network knobs (Phase 1.10). Applied to outbound
    // packets only — inbound is left untouched because the client side
    // does its own simulation (see CLI on the client in Phase 2). All
    // zero = pass-through (production default).
    struct NetSimSettings {
        float lossProbability = 0.0f; // 0.0 to 1.0
        uint32_t baseDelayMs  = 0;    // constant added to every send
        uint32_t jitterMs     = 0;    // random uniform [-jitter, +jitter]
    };
    void setNetSimSettings(const NetSimSettings& s) { m_NetSim = s; }
    NetSimSettings netSimSettings() const { return m_NetSim; }

    // Init ENet + bind the UDP listening port. Returns false on error.
    bool start(uint16_t port = kDefaultPort, size_t maxClients = 32);

    // Tear down. Safe to call multiple times.
    void stop();

    // Drive one frame of network + simulation. `frameDeltaSeconds`
    // is real time elapsed since the previous call. Internally uses
    // a fixed-timestep accumulator and runs zero, one or many 128 Hz
    // ticks depending on how much time has passed.
    void step(double frameDeltaSeconds);

    // Metrics / introspection (read-only, lock-free).
    Tick currentTick() const { return m_Tick; }
    size_t numPlayers() const { return m_Players.size(); }
    uint16_t port() const { return m_Port; }
    bool isRunning() const { return m_Host != nullptr; }
    uint64_t packetsSent() const { return m_PacketsSent; }
    uint64_t packetsRecv() const { return m_PacketsRecv; }
    uint64_t lagCompHits() const { return m_LagCompHits; }

private:
    // ENet event dispatchers — defined in the .cpp where the real types
    // are visible.
    void onConnect(void* peer);
    void onDisconnect(void* peer);
    void onPacket(void* peer, const uint8_t* data, size_t len);

    // Per-tick logic.
    void simulateTick();
    void broadcastSnapshot();
    void sendTo(void* peer, const std::vector<uint8_t>& bytes, uint8_t channel);

    // Apply one validated input to its player's state. Clamps inputs.
    void applyInput(PlayerState& ps, const InputState& in);

    // Phase 1.8: when an input arrives with the Fire bit set, rewind the
    // world to the shooter's view-time and resolve a raycast against
    // every other player's rewound hitbox. Returns the victim, if any.
    void handleFire(PlayerId shooterId, const InputState& in);

    bool m_EnetInitialized  = false;
    void* m_Host            = nullptr; // ENetHost*
    uint16_t m_Port         = 0;

    PlayerId m_NextPlayerId = 1;
    std::unordered_map<void*, PlayerId> m_PeerToPlayer; // ENetPeer* -> id
    std::unordered_map<PlayerId, PlayerState> m_Players;

    // Pending inputs received this frame, indexed by player id, dispatched
    // by the next simulateTick(). A real production server would queue
    // these per-tick using the client's tick number; we batch by frame
    // for simplicity in Phase 1.5.
    std::unordered_map<PlayerId, std::vector<InputState>> m_Pending;

    // Fixed-timestep accumulator. We run as many simulation ticks per
    // frame as the elapsed time allows.
    double m_Accumulator = 0.0;
    Tick m_Tick          = 0;

    // Metrics
    uint64_t m_PacketsSent = 0;
    uint64_t m_PacketsRecv = 0;

    // Phase 1.8: per-player rewindable position history + hit registration.
    LagCompensation m_LagComp;
    double m_ServerTimeSec = 0.0; // monotonic server wall time in seconds

    // For debug log: total hits the server has resolved since start.
    uint64_t m_LagCompHits = 0;

    // Phase 1.10: simulated bad-network knobs + deferred-send queue.
    // When `m_NetSim` has any non-zero field, sendTo() drops or buffers
    // packets instead of handing them straight to ENet.
    NetSimSettings m_NetSim;
    struct DeferredPacket {
        void* peer;
        std::vector<uint8_t> bytes;
        uint8_t channel;
        double releaseAtServerSec;
    };
    std::vector<DeferredPacket> m_DeferredOut;
    uint64_t m_NetSimDropped = 0;
};

} // namespace Net
