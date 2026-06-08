# ADR-003 — Listen-server architecture & server-side input clamping

- **Status:** Accepted
- **Date:** 2026-05 (Phase 1.5)
- **Phase:** 1.5
- **Supersedes:** none
- **Superseded by:** none
- **Related:** [ADR-002 — Netcode architecture](0002-netcode-architecture.md)

## Context

Phase 1.4 wired the `NetworkClient` into `Application`. We now need a
server. Two shapes of the same server were chosen up-front in ADR 0002:

1. A **standalone** `trueshot_server` binary — runs on a dedicated box,
   no rendering, no audio, no window.
2. A **listen-server** — one of the clients embeds `Net::Server` in the
   same process and plays at the same time. This is what casual users
   want for LAN play ("just click Host & Play").

Both modes need to share **the same** authoritative simulation, the same
tick rate (128 Hz), the same input handling, and the same anti-cheat
posture. Forking the server logic into two implementations would be a
maintenance disaster and an anti-cheat hole (different code paths =
different bug surface).

Separately: Phase 9 will deploy a custom kernel-mode anti-cheat ("best
of industry, even if it takes 3 years" — owner directive). The kernel
AC catches cheats at the OS level, but the **server is the second line
of defense** — it must reject impossible inputs even if a perfectly
crafted packet bypasses every userland check. Phase 1 needs to lay that
foundation, not retrofit it later.

## Decision

### 1. `Net::Server` is a **library**, not just an executable

`network_module/src/Server.cpp` builds into `trueshot_net` (the shared
library). `network_module/src/main_server.cpp` is a 40-line entry point
that owns one `Net::Server` and runs the tick loop. The library can be
linked into:

- the standalone `trueshot_server` executable, and
- the client (eventually — Phase 2 wires "Host & Play" through the same
  `Net::Server` instance running on its own thread).

The library-vs-executable split is **mandatory**: compiling `Server.cpp`
into both the library and the executable causes an ODR violation that
MSVC silently accepts in Debug but breaks in LTO/Release.

### 2. Refcounted `enet_initialize`

Both `NetworkClient` and `Net::Server` need ENet initialised. In
listen-server mode, both objects coexist in the same process. The fix:

- Each ctor calls a static `enet_acquire()` helper that atomically
  refcounts; the first acquire calls `enet_initialize()`.
- Each dtor calls `enet_release()`; the last release calls
  `enet_deinitialize()`.

This is invisible to callers and avoids the classic double-init / early-
deinit bug.

### 3. Hard input clamping on every packet — **non-negotiable**

Every `InputState` received by the server is clamped on arrival, before
being fed to the simulation:

```cpp
in.moveForward = std::clamp(in.moveForward / 127.0f, -1.0f, 1.0f);
in.moveRight   = std::clamp(in.moveRight   / 127.0f, -1.0f, 1.0f);
in.yaw   = std::clamp(in.yaw,   -180.0f, 180.0f);
in.pitch = std::clamp(in.pitch,  -89.0f,  89.0f);
```

This is **not optional defensive programming**. It is part of the anti-
cheat posture:

- A cheater sending `moveForward = 50.0` cannot speedhack — the server
  treats it as `1.0`.
- A cheater sending `pitch = 720.0` cannot look through walls via
  out-of-range angle exploits — the server treats it as `89.0`.
- Future input fields (fire rate, jump request frequency) will follow
  the same pattern: every value the server reads from a packet is
  bounded by a sanity check that matches what the legitimate client
  could possibly produce.

The kernel anti-cheat in Phase 9 will catch packet-crafting cheats at
a different layer. The server clamp is the **safety net** that means
even a perfect bypass of every other layer still yields a non-cheating
gameplay outcome.

### 4. Per-peer `lastAckedSeq`

Each `PlayerState` tracks the last input `seq` the server has consumed
for that peer. The Snapshot broadcast to that peer carries that peer's
`ackSeq` (not a global one). This lets the client in Phase 1.7 replay
exactly the unacknowledged inputs from `lastAckedSeq + 1` onward.

### 5. Tick loop independent of `step()` cadence

The server runs a 128 Hz fixed accumulator that is **decoupled** from
the rate at which `Server::step(dt)` is called. The standalone binary
calls `step()` in a tight loop with a 500 µs sleep; a listen-server
will call it from the game's main loop at whatever frame rate that
loop runs at. Both produce identical simulation output because the
accumulator catches up internally.

The accumulator is capped at 0.25 s to prevent the spiral of death if
the host stalls (e.g. a 5 s GC pause would otherwise cause the server
to "make up" 640 ticks at once).

## Consequences

### Positive

- One simulation, two hosting models, zero forked code.
- Anti-cheat foundation is in the codebase **from day one of Phase 1**,
  not retrofitted at Phase 9.
- Listen-server lands "for free" in Phase 2 — the only new code is the
  thread wiring.
- ODR-safe library/executable split that holds up under MSVC LTO.

### Negative

- Two compile targets in `network_module/CMakeLists.txt` (library +
  executable) instead of one. Acceptable.
- The refcounted `enet_initialize` adds a tiny bit of static state.
  Documented in `NetworkClient.h` and `Server.h`.

### Neutral

- The hard clamp will eventually need to be sourced from a config so
  that mod servers can tweak (e.g. `sv_maxspeed`). For now it's
  hardcoded — same values for everyone, no surprise behaviour.

## Alternatives considered

### Make `Net::Server` an interface, ship two implementations

Rejected. Two implementations = two anti-cheat surfaces. Not worth the
"clean architecture" win.

### Run the listen-server in a child process

Considered. Pros: rock-solid isolation, OS-level crash containment.
Cons: doubles RAM (assets loaded twice), adds IPC latency for shared
state. Not worth it for LAN play. Reconsider in Phase 8 if dedicated
servers ever colocate with clients.

### Soft input clamping (log + reject)

Rejected. Logging "bad input" gives the cheater feedback that the
server noticed. Clamping silently is the right posture — the cheat
produces no effect, the cheater can't tell which value tripped the
check.
