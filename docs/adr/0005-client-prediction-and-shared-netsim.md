# ADR-005 — Shared `NetSim` step + client prediction with reconciliation

- **Status:** Accepted
- **Date:** 2026-06 (Phase 1.7)
- **Phase:** 1.7
- **Supersedes:** none
- **Superseded by:** none
- **Related:** [ADR-002 — Netcode architecture](0002-netcode-architecture.md),
  [ADR-003 — Listen-server & input clamping](0003-listen-server-and-input-clamping.md),
  [ADR-004 — Snapshot interpolation](0004-snapshot-interpolation.md)

## Context

Phase 1.6 wired snapshot interpolation for remote players. The local
player is a different problem: rendering it 100 ms in the past would
make WASD feel sluggish at any non-zero RTT. We need **client-side
prediction** — the client applies its own inputs immediately so the
player sees their character move with zero perceived input lag, and
reconciles against the server's authoritative state when each Snapshot
arrives.

Two sub-problems fall out of this:

1. **Identical simulation on both sides.** If the client predicts with
   any formula that differs from what the server runs (different
   rounding, different physics constants, different float ops in a
   different order), the reconciliation will fire on every Snapshot
   and the local player will visibly jitter forever.
2. **What to do with stale unacknowledged inputs.** Inputs sent by the
   client but not yet processed by the server need to be replayed on
   top of the authoritative state, otherwise the prediction snaps
   backwards 100 ms every Snapshot.

## Decision

### 1. Extract `Net::stepSim` as a **shared header-only** function

`network_module/include/Network/NetSim.h` exposes:

```cpp
struct SimState {
    glm::vec3 pos;
    float yaw, pitch;
    uint8_t stateFlags;
};

inline void stepSim(SimState& s, const InputState& in);
```

The body lives in the header, marked `inline`, so both the server
(`Server.cpp`) and the client (`client_prediction.cpp`,
`application.cpp`) compile against the **same translation unit
contents**. Any future edit to the formula touches exactly one place.

The Phase 1.7 implementation is intentionally **minimal** — 5 m/s flat
ground movement, hard input clamping, no jumping, no air control, no
friction, no crouch slowdown. This is enough to validate prediction +
reconciliation end-to-end without dragging the full Source-style
movement model (`PlayerController`) into the shared sim yet. Phase 2
migrates the full movement model into `NetSim` with the same "must be
identical on both sides" discipline.

`Server::applyInput` now does nothing but copy state into a `SimState`,
call `stepSim`, copy back, and bump `lastAckedSeq`. The "formula"
lives nowhere in the server.

### 2. Hard input clamping is **inside** `stepSim`, not before it

Phase 1.5 (ADR-003) clamped inputs in `Server::applyInput`. Now that
clients call `stepSim` too, the clamp must apply on both sides — both
to keep prediction identical to the server, and so a misbehaving local
client can't even *think* it moved at 100 m/s. The clamp moved into
`stepSim` itself; nothing outside the function ever sees an
unclamped move vector. This preserves the anti-cheat posture from
ADR-003 with no extra code.

### 3. `ClientPrediction` owns the ring buffer of pending inputs

A 256-entry `std::array<PendingInput, 256>` covers 2 seconds of
unacknowledged inputs at 128 Hz — far more than any realistic RTT.
Overflow drops the oldest silently: by then the connection is dead and
the user will get a disconnect from elsewhere.

The buffer is FIFO. Each tick:

```cpp
ClientPrediction::predict(input):
  1. stepSim(m_State, input)        // local sim immediately
  2. push(input) into the ring      // remember for replay
```

### 4. Reconciliation policy: drop, snap, replay, optionally smooth

When a `Snapshot` arrives carrying the local player's `ackSeq`:

```cpp
ClientPrediction::reconcile(ackSeq, authoritative):
  1. Drop pending inputs with seq <= ackSeq.
  2. Measure distance from m_State.pos to authoritative.pos.
  3. If distance < 2 cm AND pending is empty: prediction was right,
     just snap to authoritative and return.
  4. Replay every still-pending input on top of authoritative to
     compute the "where we should be now" state.
  5. If distance >= 50 cm: snap. The prediction is too wrong to hide.
  6. Otherwise: soft-correct — snap orientation, lerp position 25 %
     toward the corrected state. The next reconciliation continues
     the nudge; over ~5-6 ticks (~40 ms) we converge invisibly.
```

The thresholds (`kIgnoreCorrectionMeters = 0.02`,
`kSnapThresholdMeters = 0.50`, `kLerpAlpha = 0.25`) live in
`client_prediction.cpp` as `constexpr` so they're easy to find and
A/B test.

### 5. Don't smooth orientation

Yaw and pitch always snap to the corrected value, never lerp.
Orientation is what the player **just controlled** with the mouse;
smoothing it would feel exactly like mouse input lag, which is the
single worst thing an FPS can do. Position can lerp because the
correction is on a tick boundary (~7.8 ms apart) and 25 % of a 2-50 cm
delta is sub-pixel at typical FOV.

### 6. The renderer doesn't read `m_Prediction` yet

Phase 1.7 lands the prediction logic but does **not** wire its output
to `FPSCamera`. The `PlayerController` continues to drive the camera
in client mode — the prediction tracks in parallel and is observable
via the debug log (`pending`, `predPos`, `lastCorr`).

This is deliberate. Replacing the camera source touches the
audio/weapon/HUD pipeline (every system that reads camera position),
and it's worth doing in a focused PR rather than mixing it with the
prediction algorithm. Tracked as Phase 1.7b / 1.9 in `ROADMAP.md`.

## Consequences

### Positive

- Identical formula on both sides eliminates "constant mispredict"
  jitter at the source.
- The reconciliation algorithm is decoupled from physics — when Phase
  2 swaps in the full Source-style sim, the reconciliation code
  doesn't change.
- Anti-cheat posture from ADR-003 still holds: the clamp is inside
  the formula, the server can't be tricked into running with unclamped
  inputs even if a future caller forgets to clamp first.
- 2 seconds of pending buffer is comically generous on LAN and still
  fine on a 200 ms WAN connection.

### Negative

- The "minimal sim" in Phase 1.7 does **not** match `PlayerController`
  visually. In client mode, what you see (driven by `PlayerController`)
  and what the server thinks (driven by `NetSim`) differ. Phase 1.7's
  debug log surfaces this gap. Phase 2 closes it by promoting
  `PlayerController` into the shared sim.
- The "soft correction" policy is one of the most game-feel-sensitive
  knobs in the whole codebase. Expect to tune `kLerpAlpha` and
  `kSnapThresholdMeters` after Phase 1.10 LAN testing.

### Neutral

- We don't carry velocity on the wire yet. That's fine for the
  minimal sim (no acceleration, no air strafing). When the full sim
  arrives we'll need to either include velocity in the snapshot or
  derive it deterministically from inputs.

## Alternatives considered

### Run the renderer's `PlayerController` as the predicted sim

Rejected. `PlayerController` is non-deterministic across machines
today (it depends on frame timing, accumulator residuals, mouse
smoothing, etc.). Trying to make it bit-identical would have meant
freezing its internals before they're done evolving. Easier to start
with a tiny shared sim and grow it.

### Predict without reconciliation, trust the snapshot timing

Rejected on day one. Without reconciliation, any clock drift between
client and server (which is constant — every machine drifts) means
the predicted position diverges forever. Plus, server-authoritative
gameplay requires the server to be able to overrule the client.

### Lerp orientation too, "for consistency"

Rejected — see section 5. Mouse input lag is unacceptable in a
competitive FPS; position smoothing is invisible, orientation
smoothing is fatal.
