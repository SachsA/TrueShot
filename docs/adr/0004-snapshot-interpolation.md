# ADR-004 — Snapshot interpolation for remote players

- **Status:** Accepted
- **Date:** 2026-06 (Phase 1.6)
- **Phase:** 1.6
- **Supersedes:** none
- **Superseded by:** none
- **Related:** [ADR-002 — Netcode architecture](0002-netcode-architecture.md)

## Context

After Phase 1.5, the client receives a 128 Hz stream of `Snapshot`
packets describing every entity's authoritative position. The client
now needs to **render** those entities. Three obvious approaches:

1. **Render the latest snapshot directly.** Every remote player teleports
   each time a packet arrives. Looks broken at any non-zero packet loss.
2. **Render with prediction (dead reckoning).** The client extrapolates
   forward from the latest snapshot using a velocity estimate. Smooth
   when correct, catastrophic on quick aim changes (the predicted
   trajectory points at where the player *was* heading, not where they
   actually went). Source/Quake III famously refused this approach.
3. **Render in the past, interpolating between two known good
   snapshots.** Costs ~100 ms of perceived "lag" on remote players,
   but every position rendered is a position the server actually
   confirmed. This is what CS, Valorant, Overwatch and every other
   competitive FPS does.

We pick option 3.

## Decision

### 1. Render 100 ms behind the latest snapshot

```cpp
constexpr double kInterpDelaySeconds = 0.100;
```

At 128 Hz, 100 ms = ~12-13 snapshots in the buffer at any time. A
single lost packet is invisible; two consecutive losses are still
recoverable as long as one snapshot lands within the window. The
trade-off:

| Delay | Robustness | Felt latency |
|---|---|---|
| 50 ms  | Survives ~6 dropped packets | Snappier remote movement |
| 100 ms | Survives ~12 dropped packets | Industry standard |
| 200 ms | Survives ~25 dropped packets | Sluggish, peeker's advantage worsens |

100 ms is the canonical FPS choice. Phase 1.10's LAN test pass will
confirm it under simulated packet loss; the value is centralized in
one `constexpr` for easy A/B tuning.

### 2. Ring buffer of 64 samples per remote player

```cpp
static constexpr size_t kRingSize = 64;
std::array<RemotePlayerSample, kRingSize> m_Ring{};
```

64 samples = 500 ms of history at 128 Hz. That's 5× the interp delay,
which is enough headroom to absorb any reasonable jitter spike without
hitting the back of the buffer. A `std::deque` would also work but
allocates; `std::array` is contiguous, cache-friendly, and never
allocates after construction.

The pruning policy keeps anything within `4 × kInterpDelaySeconds` of
the current render time (= 400 ms). Older samples are dropped on
write — there's no useful reason to keep them.

### 3. Linear interpolation between two enclosing snapshots

We walk the ring from newest to oldest, find the pair `(newer, older)`
such that `older.timestamp ≤ renderTime ≤ newer.timestamp`, and lerp
between them:

```cpp
const double span = newer->timestamp - older->timestamp;
float t = static_cast<float>((renderTime - older->timestamp) / span);
outPos = glm::mix(older->pos, newer->pos, t);
```

Yaw and pitch interpolate the same way. Flags (alive/crouched) snap
to the newer sample — there's no "half-dead" pose.

### 4. **Freeze, don't extrapolate**, on starvation

When we have an `older` sample but no `newer` one (the most recent
snapshot is in the past — we've fallen behind), we hold the older
pose:

```cpp
if (!newer) {
    outPos    = older->pos;
    outYaw    = older->yaw;
    outPitch  = older->pitch;
    outFlags  = older->stateFlags;
    return (outFlags & EntityFlag::Alive) != 0;
}
```

This is **deliberate**. Phase 1.6 does not extrapolate. The reasoning:

- We don't carry velocity in the snapshot yet (Phase 1.7 may add it).
- Extrapolating from inferred velocity at low confidence produces the
  exact failure mode mentioned above (player rubber-bands in a
  direction they aren't going).
- A frozen pose for ~100 ms during a packet storm is **less wrong**
  than a confidently extrapolated wrong pose.

`kExtrapolationMaxSeconds = 0.100` is reserved as a knob for Phase 1.10
if testing shows the freeze is too jarring under bad network. Until
then we leave it unused.

### 5. The local player is **never** rendered from snapshots

`RemotePlayerRegistry::ingestSnapshot()` skips any entity whose `id`
equals the local player's. The local player is rendered from
**prediction** (Phase 1.7), not from snapshots, because the 100 ms
delay would make local input feel sluggish.

This is the standard "client predicts self, interpolates others"
split that every competitive FPS uses.

### 6. Placeholder rendering — cubes for now

`Renderer::drawRemotePlayers()` emits a vertical cube
(`scale(0.8, 1.8, 0.4)`, rough humanoid silhouette) at each
interpolated pose. Real character models, animations and skeletal
rigs land in Phase 4. This keeps Phase 1's surface area focused on
the network layer.

## Consequences

### Positive

- Every position rendered is a server-confirmed truth.
- A single dropped packet is invisible at any reasonable jitter.
- Cache-friendly fixed-size ring buffer — zero allocations after
  registry construction.
- Local-vs-remote rendering split is clean: prediction for self,
  interpolation for others, no code paths cross.

### Negative

- Remote players are perceived 100 ms behind their actual server
  position. This is the "peeker's advantage" — the player rounding a
  corner sees the defender ~100 ms before the defender sees them.
  Lag compensation in Phase 1.8 compensates for this on the **tir**
  side (the shooter rewinds the world to where the defender *was* on
  the shooter's screen at fire time).

### Neutral

- Phase 1.7's reconciliation needs to know about the snapshot stream,
  but it consumes the raw `Snapshot` events directly (for the local
  player's ackSeq), not through `RemotePlayerRegistry`. The two
  consumers can coexist.

## Alternatives considered

### Hermite interpolation between snapshots

Smoother on direction changes than linear. Rejected for Phase 1
because:

- Hermite needs a velocity estimate or the next-next sample for
  tangent estimation, neither of which is on the wire yet.
- The visual win is invisible at 128 Hz — samples are already 7.8 ms
  apart, so linear segments between them are sub-pixel at any
  reasonable distance.

Revisit in Phase 4 if animation blending exposes the discontinuity.

### Render the future (extrapolate forward)

Rejected for the reasons in section 4 above. The industry rejected
this approach 25 years ago and nothing has changed about the
underlying maths.

### Adaptive interp delay (smaller when network is good)

Considered. Pros: snappier remote feel on LAN. Cons: any visible
change in remote player smoothness as the delay shrinks/grows would
read as "the game is broken." Players prefer consistent feel over
peak feel. The CS, Valorant, Overwatch consensus is a fixed delay;
we follow that.
