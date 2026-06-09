#pragma once

#include <glm/glm.hpp>

#include "Network/NetCommon.h"

// ---------------------------------------------------------------------
// NetSim — the **shared** authoritative simulation step.
//
// This is the bit of physics that runs both:
//   - on the server, every tick, for every connected peer (truth);
//   - on the client, every tick, for the local player (prediction).
//
// It MUST be byte-identical between the two so that client-side
// prediction matches what the server eventually says happened. Any
// drift here will surface as constant reconciliation snaps.
//
// Phase 1.7 is intentionally a *minimal* shared sim — 5 m/s flat ground
// movement, no jump, no air-control, no friction. The full Source-style
// movement (PlayerController) replaces it in Phase 2 once we extract
// that into a deterministic, allocation-free, sandboxed simulation that
// can run identically on both sides.
//
// See docs/adr/0002-netcode-architecture.md.
// ---------------------------------------------------------------------

namespace Net {

// Tuneables — duplicated in tests to spot accidental edits.
constexpr float kNetSimMoveSpeed = 5.0f; // units / second
constexpr float kNetSimFixedDt   = 1.0f / 128.0f;

// Pure state we simulate. Matches the server's `PlayerState` subset
// used by `applyInput` — explicitly *not* the renderer's PlayerState
// (no camera, no audio).
struct SimState {
    glm::vec3 pos{0.0f};
    float yaw          = 0.0f;
    float pitch        = 0.0f;
    uint8_t stateFlags = 0;
};

// Step one tick. Called once per simulation tick (128 Hz) on both
// client and server. Inputs are clamped here, so neither side can
// produce out-of-bounds state.
inline void stepSim(SimState& s, const InputState& in) {
    // Hard clamp — same posture as the server. Identical formula =
    // identical output bits.
    const float fwd    = (in.moveForward / 127.0f);
    const float rgt    = (in.moveRight / 127.0f);
    const float fwdC   = (fwd < -1.0f) ? -1.0f : (fwd > 1.0f ? 1.0f : fwd);
    const float rgtC   = (rgt < -1.0f) ? -1.0f : (rgt > 1.0f ? 1.0f : rgt);

    const float yawRad = in.yaw * 3.1415926535f / 180.0f;
    const float cosY   = static_cast<float>(::cos(yawRad));
    const float sinY   = static_cast<float>(::sin(yawRad));

    s.pos.x += (fwdC * cosY + rgtC * sinY) * kNetSimMoveSpeed * kNetSimFixedDt;
    s.pos.z += (fwdC * sinY - rgtC * cosY) * kNetSimMoveSpeed * kNetSimFixedDt;

    s.yaw        = (in.yaw < -180.0f) ? -180.0f : (in.yaw > 180.0f) ? 180.0f : in.yaw;
    s.pitch      = (in.pitch < -89.0f) ? -89.0f : (in.pitch > 89.0f) ? 89.0f : in.pitch;

    s.stateFlags = EntityFlag::Alive | EntityFlag::OnGround;
    if (in.buttons & InputButton::Crouch) s.stateFlags |= EntityFlag::Crouching;
    if (in.buttons & InputButton::ADS) s.stateFlags |= EntityFlag::Aiming;
    if (in.buttons & InputButton::Fire) s.stateFlags |= EntityFlag::Firing;
    if (in.buttons & InputButton::Reload) s.stateFlags |= EntityFlag::Reloading;
}

} // namespace Net
