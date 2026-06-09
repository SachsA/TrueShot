# ADR-006 — Lag compensation for server-side hit registration

- **Status:** Accepted
- **Date:** 2026-06 (Phase 1.8)
- **Phase:** 1.8
- **Supersedes:** none
- **Superseded by:** none
- **Related:** [ADR-002 — Netcode architecture](0002-netcode-architecture.md),
  [ADR-003 — Listen-server & input clamping](0003-listen-server-and-input-clamping.md),
  [ADR-004 — Snapshot interpolation](0004-snapshot-interpolation.md),
  [ADR-005 — Shared NetSim + client prediction](0005-client-prediction-and-shared-netsim.md)

## Context

The peeker's advantage problem (ADR-004): a remote player is rendered
on the shooter's screen ~100 ms behind their true server position, plus
the shooter's outgoing input takes ~RTT/2 to reach the server. By the
time the server processes the `Fire` input, the victim has potentially
moved out of the AABB the shooter aimed at.

Without compensation:

- The shooter aims dead-center on their screen.
- The shot lands "behind" the target on the server.
- The shooter rage-quits because their aim was correct on their screen.

This is unacceptable for a competitive FPS. Industry-standard fix:
**lag compensation** — the server rewinds the world to the moment the
shooter actually fired (from their perspective), runs the hit test
against rewound positions, then accepts/rejects the hit based on
geometry. This is what CS, Valorant, Quake, Overwatch, and every
other competitive shooter does.

The implementation must also defend against the **"fake-ping" exploit**:
a cheater deliberately reports an inflated `clientPingMs` to extend the
rewind window, hoping the server will rewind far enough that the target
hadn't yet rounded the corner.

## Decision

### 1. Per-player history ring buffer (128 samples × 7.8 ms = 1 s)

```cpp
class PlayerHistory {
    static constexpr size_t kRingSize = 128;
    std::array<HistorySample, 128> m_Ring;
    ...
};
```

128 samples at 128 Hz = 1 second. The actual rewind cap is 200 ms, so
we have 5× headroom — plenty for any reasonable jitter or interp
delay variation.

Each sample carries the canonical `pos / yaw / pitch / stateFlags` at
a specific server wall time (`tServer`, monotonic, advanced once per
simulation tick).

Recording is automatic: after every `Server::simulateTick` (i.e. after
`applyInput` has run for every pending input on every peer), we push
one fresh sample per player. The shooter's own pose for the tick of
fire is therefore already in the history.

### 2. View-time formula

```cpp
double computeViewTime(double tServerNow, uint16_t clientPingMs) {
    const double halfRtt = clientPingMs * 0.001 * 0.5;
    return tServerNow - halfRtt - kClientInterpDelaySeconds;  // 100 ms
}
```

The three terms:

| Term | What it accounts for |
|---|---|
| `tServerNow` | The instant the server receives the Fire input. |
| `−halfRtt` | One-way latency: when the input *was actually pressed* on the client, the server's clock was earlier by RTT/2. |
| `−kClientInterpDelay` | The shooter's local renderer was already 100 ms behind the latest snapshot for remote players. |

`kClientInterpDelaySeconds` is duplicated as a `constexpr` in
`LagCompensation.cpp` rather than `#include`d from the client header,
because the server module must not depend on the game-side include
tree. The two constants **must** stay in sync; a static assertion
across the boundary is added in Phase 1.10 when we wire up the test
harness.

### 3. Sample lookup = linear interpolation between two enclosing samples

`PlayerHistory::sampleAt(tTarget)` walks newest → oldest, finds the
two samples enclosing `tTarget`, and lerps between them. This is the
same logic as `RemotePlayer::sample` (ADR-004), just running on the
server side. Refuses (returns false) if `tTarget` predates our oldest
sample — better to skip the hit test than to extrapolate backwards
into garbage.

### 4. Hitbox = single AABB matching the placeholder render

Phase 1.8 uses a single axis-aligned box per player:

```cpp
PlayerHitbox makeHitbox(playerPos) {
    halfWidth = 0.40, halfDepth = 0.20, height = 1.80
    mins = (x - 0.40, y,        z - 0.20)
    maxs = (x + 0.40, y + 1.80, z + 0.20)
}
```

This matches the placeholder cube drawn by `Renderer::drawRemotePlayers`
(scale 0.8 × 1.8 × 0.4). When real character models arrive in Phase 4,
this gets replaced by bone-driven hitboxes (head / chest / arms /
legs) with proper damage zones — but the **lag-compensation algorithm
doesn't change**, only the geometry it tests against.

### 5. Ray-vs-AABB slab test

Standard slab algorithm — six axis tests, early exit on the first
miss, parallel-ray special case. ~30 ns per box on modern hardware,
so the cost of "rewind every player and test" is negligible up to
10v10.

### 6. **Hard 200 ms rewind cap** (`kRewindCapSeconds = 0.200`)

If `tServerNow - tView > 200 ms`, the server refuses the rewind and
returns `std::nullopt` (no hit registered). The rationale is **purely
anti-cheat**:

- 200 ms covers any legitimate North America ↔ Europe play.
- Beyond that, the most plausible explanation for the latency is that
  the client is deliberately spiking ping to gain a wider rewind
  window — the "lag for free shots" exploit.
- A genuinely lagging player still gets to play (their inputs are
  processed, their movement is replicated), they just lose hit
  registration parity. That's the right trade-off: a fair fight to a
  laggy player is better than an unfair fight that everyone else
  loses.

This is a Phase 1 commitment to the anti-cheat posture set in ADR-003:
the server takes no client value at face value; everything has a
sanity cap.

### 7. Phase 1.8 stops at "log the hit"

`handleFire` resolves the raycast and logs the resolved hit:

```text
[Server] LAG-COMP HIT shooter=1 victim=2 dist=4.3m ping=42ms
```

It does **not** apply damage, broadcast a `PlayerHit` event, update a
kill feed, or change the player's `stateFlags`. Those land in Phase 2
when we have a match structure (HP, rounds, score). Splitting it this
way lets Phase 1 finish on netcode plumbing only — Phase 1.10's LAN
test pass validates the rewind math by reading the log line, no game-
logic refactor needed.

## Consequences

### Positive

- Hit registration matches what the shooter saw on their screen,
  regardless of RTT (up to 200 ms).
- Algorithm is independent of hitbox geometry — Phase 4 swaps the
  AABB for skeleton hitboxes without touching the rewind code.
- The 200 ms cap closes the "fake-ping for free shots" cheat by
  construction.
- 1 second of history is comically generous, so the policy can be
  tuned later (e.g. to 150 ms cap for ranked) without rebuilding.

### Negative

- Single AABB hitbox in Phase 1.8 means there are no headshots yet.
  Acceptable — headshots are a Phase 4 (with real models) concern,
  and the Phase 1 LAN tests don't exercise them.
- We record one sample per player per tick *unconditionally*. At
  128 Hz × 32 players that's 4 KB/s of state in RAM — trivial. But it
  means standing-still players still get recorded, which is wasted
  for "did the shot hit?" but useful for "where was the player
  during this whole encounter?" debugging. Net positive.

### Neutral

- We don't yet cross-check `clientPingMs` against the RTT the server
  measures via ENet. That's a Phase 2 anti-cheat strengthening:
  refuse hits where the gap between reported and measured ping
  exceeds, say, 25 ms. Tracked in ROADMAP §1.8 backlog.

## Alternatives considered

### "Trust the client to do hit detection and report hits"

Rejected with extreme prejudice. This is the cheater's paradise. The
server is authoritative or it isn't — and "it isn't" was decided
against in ADR-002.

### Rewind only the targets the shooter named

Considered — the shooter could pre-declare "I'm shooting player N",
and we'd only rewind N. Saves CPU. Rejected because (a) at 10
players the rewind is microseconds, and (b) the wire-trust required
("you said you shot N") creates a new cheat vector: claim you shot
someone who isn't there.

### Adaptive rewind cap (smaller when network is good)

Considered — could tighten the cap when the shooter has a stable
low ping and widen it when they have a stable high ping. Rejected
for Phase 1 because the policy interaction with the anti-cheat
intent ("higher ping = bigger rewind window") creates exactly the
exploit we're trying to prevent. A fixed cap is easier to reason
about. Revisit in Phase 9 when the full anti-cheat lands.

### Per-bone hitboxes now

Rejected. We don't have character models. Doing per-bone hitboxes
against a 0.8 × 1.8 × 0.4 cube is the same as doing one AABB
against it. Pay the complexity in Phase 4 when there's something to
hit.
