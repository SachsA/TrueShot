# ADR-002 — Netcode architecture

- **Status:** Accepted
- **Date:** 2026-06-05
- **Phase:** 1 (Netcode jouable 1v1 LAN)
- **Supersedes:** none
- **Superseded by:** none

## Context

TrueShot is a competitive 5v5 FPS. The netcode is the foundation that
everything else depends on — movement feel, hit registration, anti-cheat,
e-sport spectator. We need an architecture that is:

1. **Authoritative** — the server is the single source of truth for
   positions, hits and game state. This is non-negotiable for a
   competitive FPS: client-side trust kills the game on day one.
2. **High-tick** — 128 Hz is the de-facto pro standard
   (CS2 / FACEIT / Valorant). Anything below feels laggy at the top
   level and is mocked by the community.
3. **Latency-tolerant** — prediction + reconciliation on the client,
   interpolation on remote entities, lag compensation on shots. A 50-
   to 100-ms ping must feel native.
4. **Portable** — Windows + macOS + Linux, same binary except for the
   Winsock + winmm system libs (already handled at link time).

## Decision

### Transport

- **ENet over UDP** — already in the repo, mature, cross-platform, comes
  with reliability/sequencing per channel.
- Two ENet channels:
  - **Channel 0 — Reliable ordered:** handshake, RPC, configuration,
    disconnect.
  - **Channel 1 — Unreliable sequenced:** `ClientInput`, `Snapshot`.
    Dropped packets are absorbed by the prediction/interpolation
    layers; no point in retransmitting state that is already stale.

### Tick rate

- **128 Hz fixed** on both client and server simulation.
  (`TICK_RATE = 128.0f`, `FIXED_TIMESTEP = 1.0f / 128.0f`.)
- A `TickClock` class drives the loop on both sides. The server `sleep`s
  until the next tick boundary; the client interleaves rendering with
  fixed-timestep simulation steps via an accumulator.
- Tick rate is read from `physics_types.h` so it can be changed in one
  place. Per-tick costs and packet sizes are budgeted assuming 128 Hz —
  we won't ship at 64 Hz, but the code stays correct if we ever switch.

### Authority

- **Server authoritative from day one.**
  - Client sends `ClientInput { tick, seq, move, view, buttons }` at
    every tick.
  - Server applies inputs in the player's local frame to the canonical
    `PlayerState`, runs the fixed-timestep physics, and broadcasts
    `Snapshot { tick, entities[] }` at the same rate.
- The client **never sends positions**. The server clamps move/view
  ranges before applying them — this is the foundation that future
  anti-cheat (Phase 9) builds on.

### Client prediction

- Client applies its own inputs **immediately** to a predicted state
  and renders from that. No 1-tick visible latency on local move.
- Client keeps a ring buffer of unacknowledged inputs (last ~1 s ~
  128 entries).
- When a `Snapshot` arrives, the client:
  1. Drops inputs older than the server's last acknowledged `seq`.
  2. Compares server's authoritative state for the local player vs.
     the predicted state at the matching tick.
  3. If the divergence is small (< 5 cm position, < 1° angle), the
     client smooths over a few ticks.
  4. If it's large (> 50 cm), the client snaps and re-applies all
     unacknowledged inputs from the authoritative state — that's the
     reconciliation step.

### Remote entity interpolation

- Each remote player has a small buffer of snapshots (last ~250 ms).
- Rendering happens **100 ms behind** the latest received snapshot
  (interp delay), giving us 2–3 snapshots to lerp between even with
  one packet drop.
- If a snapshot is missing past the interp window, extrapolate linearly
  for up to 100 ms, then snap.

### Lag compensation (shooting)

- Server keeps a 1-second history of all player positions, sampled at
  every tick.
- When a client fires, the packet includes the client's interpolation
  delay + estimated RTT.
- Server rewinds every alive entity to `(now − clientPing − interpDelay)`,
  re-runs the raycast at that historical frame, and applies damage from
  the result. Then it restores everyone to the current tick.
- Rewinds capped at 200 ms — beyond that the server uses the most stale
  acceptable state (anti-laggers exploit).

### Local player visibility

- The local player has **no visible model** to himself. Only the view
  model (weapon) is rendered. This is the standard CS / Valorant choice
  and avoids skeletal animation + clipping issues. Other players see a
  remote-model that the server replicates from their `PlayerState`.

### Hosting model

- **Listen-server** for development and casual play: one of the clients
  starts an embedded server thread, others join via LAN/IP.
- The same `Server` class powers both the listen-server and the
  dedicated `trueshot_server` executable (already in `network_module/`).
- Phase 8 will swap the listen-server for cloud-hosted dedicated servers
  for ranked play; the protocol stays identical.

### Packet schema (v1)

All numbers are little-endian. Positions/velocities use **fixed-point
16.16** for compactness and determinism. View angles use uint16
quantised over [−180°, +180°] / [−90°, +90°].

```
ClientInput  (channel 1, unreliable sequenced):
    uint8   type       = 0x01
    uint8   protocol   = 1
    uint32  tick       (client local tick)
    uint32  seq        (monotonic, used for ACK)
    int8    moveForward (-127..127)
    int8    moveRight   (-127..127)
    int16   yaw        (Q15)
    int16   pitch      (Q15)
    uint8   buttons    (bitfield: jump, crouch, fire, ads, reload, ...)
    uint16  clientPing (ms, for lag comp)

Snapshot     (channel 1, unreliable sequenced):
    uint8   type       = 0x02
    uint8   protocol   = 1
    uint32  tick       (server tick this snapshot represents)
    uint32  ackSeq     (last ClientInput seq applied for this peer)
    uint8   entityCount
    repeat entityCount times:
        uint16  playerId
        int32   posX  (16.16 fixed)
        int32   posY
        int32   posZ
        int16   yaw
        int16   pitch
        uint8   stateFlags  (alive, crouching, firing, reloading, ...)

Handshake    (channel 0, reliable):
    uint8   type       = 0x00
    uint8   protocol   = 1
    uint8   clientVersionMajor
    uint8   clientVersionMinor
    char    playerName[16]
    uint8   slot              (set by server in response)
```

Versioning: any change to a field, ordering, or width bumps `protocol`.
The handshake refuses mismatched protocols cleanly.

### Code structure (incremental delivery)

- `include/net/tick_clock.h` + `.cpp` — Phase 1.1
- `network_module/include/Network/Bitstream.h` — extended for vec3 +
  Q15 + fixed-point — Phase 1.2
- `network_module/include/Network/PacketTypes.h` — packet structs —
  Phase 1.3
- `include/net/network_client.h` + `.cpp` — Phase 1.4
- `network_module/include/Network/server_core.h` + `.cpp` — refactor of
  Server.cpp into a reusable class — Phase 1.5
- `include/net/remote_player.h` — Phase 1.6
- Prediction/reconciliation inside `NetworkClient` + `PlayerController`
  — Phase 1.7
- Lag compensation inside the server — Phase 1.8
- HUD network panel — Phase 1.9
- Manual 1v1 LAN test session — Phase 1.10

## Consequences

### Positive

- True competitive netcode from day one — no rewrite later.
- 128 Hz means we can advertise "128 tick servers, like the pros" from
  the first playable build. That's a differentiator for the community.
- Server-authoritative architecture is the contract every later phase
  builds on: anti-cheat (Phase 9), matchmaking (Phase 7), replays
  (Phase 14), spectator (Phase 14), demos.
- Clean separation client/server: the listen-server is the same Server
  class running in a thread, so the protocol stays identical from
  development → production.

### Negative

- Higher bandwidth: 128 Hz × ~64 bytes/snapshot/entity × 10 entities ≈
  82 kB/s up, 82 kB/s down per client peak. Fine on broadband, may
  squeeze low-end connections (mobile tethering). Mitigated later by
  delta compression (Phase 1 ships full snapshots; delta is a Phase 2
  optimisation if needed).
- Higher server CPU: 2× more ticks than 64 Hz. We have to be careful
  about per-tick cost from the start (no allocations in the hot loop,
  bounded data structures).
- Prediction + reconciliation is the hardest code in the project. We
  isolate it in `NetworkClient` and unit-test it in CI (Phase 1.4 will
  add a `network-test` job).

### Risks & mitigation

| Risk | Mitigation |
|---|---|
| Packet loss in burst → reconciliation snaps | Interpolation delay 100 ms covers 1–2 dropped packets transparently. |
| Player ping > 200 ms exploits lag comp | Cap rewind at 200 ms. Above, the rewind uses the most stale acceptable state, hurting the laggy player. |
| Future tick-rate change (64 ↔ 128) | Single constant in `physics_types.h`, all timers in seconds. No hard-coded constants in the packet schema. |
| Listen-server cheating in dev | Acknowledged: until anti-cheat (Phase 9) ships, the host has theoretical power. Acceptable for closed playtests with NDA. |
| Endianness mismatches (Windows ↔ Mac/Linux) | Bitstream serialises explicitly little-endian. No `memcpy` of structs. |

### Not in scope for Phase 1

- Delta compression of snapshots (full snapshot every tick is fine for
  1v1 at 128 Hz).
- Interest management / area-of-interest culling (10 players in one
  arena — everything is in view).
- Voice chat over the same socket (Phase 2.8 uses libopus on a separate
  channel).
- Encryption of the game traffic (matters for ranked + anti-cheat —
  Phase 9).

## References

- Source engine source code (publicly leaked, descriptive purposes only)
- Glenn Fiedler — "Networked Physics" series
- Valve — "Latency Compensating Methods in Client/Server In-game
  Protocol Design and Optimization"
- Overwatch GDC talks on netcode
- Quake III source code (reference for prediction/reconciliation)
