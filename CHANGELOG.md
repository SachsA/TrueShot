# Changelog

All notable changes to TrueShot are tracked here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html)
once a first tagged release ships (`v0.1.0`, expected end of Phase 1).

Phase numbering matches [ROADMAP.md](ROADMAP.md). Decisions are detailed
in [docs/adr/](docs/adr/).

## [Unreleased]

### Added — Project governance

- **`CLAUDE.md`** — the repo's working agreement, read at the start of
  every session. Codifies:
  - the hard project constraints (Windows/macOS/Linux mandatory, Steam-only
    distribution, in-house kernel anti-cheat, 128 Hz server-authoritative
    netcode, listen-server support) so they never get re-litigated;
  - the **documentation contract**: after any code change, every affected
    doc, build config, tooling config, CI workflow, run script and ignore
    file must be audited and updated — plus the files to create when they
    become relevant (`SECURITY.md`, `CODEOWNERS`, ops runbooks, ...);
  - cross-file consistency rules (ADR numbering, phase numbering, pinned
    versions, CLI flags, shared client/server constants);
  - the commit-message format (Conventional Commits, with the scopes in
    use);
  - an 11-point definition of done;
  - the technical conventions previously scattered across `CONTRIBUTING.md`
    and code comments.
- `CONTRIBUTING.md` and `.github/pull_request_template.md` now point at
  the documentation contract as a merge gate.

### Fixed — clang-tidy CI

- `.clang-tidy`: disabled the purely-stylistic checks that fight the
  project's single-line-guard style (`readability-braces-around-statements`,
  `readability-else-after-return`, `readability-isolate-declaration`,
  `readability-make-member-function-const`,
  `readability-convert-member-functions-to-static`, `modernize-use-auto`,
  `modernize-return-braced-init-list`, `cppcoreguidelines-avoid-c-arrays`),
  each with a rationale comment. Substantive checks stay on.
- **`player_controller.cpp`: `abs()` was resolving to the `<cstdlib>`
  integer overload**, truncating the float strafe-angle delta before taking
  the absolute value. The strafe-jump bonus curve has been subtly wrong
  since Phase 0. Now `std::fabs`. Also `acos` → `std::acos`.
- `weapon_system.cpp`: `rand()` → thread-local `std::mt19937` +
  `uniform_real_distribution` (cert-msc50-cpp); `sin` → `std::sin` (no
  float→double promotion); `static_cast<float>(i)` for the recoil-pattern
  timing (narrowing); merged the identical `DRAWING`/`default` switch
  branches.
- `player_controller.cpp`: `m_GameTime` and `m_LastFootstepPos` moved into
  the constructor's member-initializer list.
- `network_client.cpp`: `PacketType type` now has a defined value before
  the `readHeader()` early-return path; merged the identical
  `Disconnect`/`default` switch branches.
- `renderer.cpp`: `(void*)0` → `nullptr`, `(void*)(offset)` →
  `reinterpret_cast<void*>(offset)`, and the three
  `glDrawElements(..., 0)` calls → `nullptr`.
- `hud.cpp`: `std::snprintf`'s return value explicitly discarded with a
  comment (cert-err33-c).
- Project-wide: `<< std::endl` → `<< "\n"` (endl forces a stream flush on
  every log line).

### Added — Phase 1.10 (Test 1v1 LAN)

- **GoogleTest suite** under `tests/` covering the netcode invariants
  cross-platform (Windows + macOS + Linux × Debug + Release in CI):
  - `test_netsim.cpp` — determinism of `stepSim`, hard clamps, button
    bitfield → `EntityFlag` mapping.
  - `test_bitstream.cpp` — round-trip for U8/U16/U32/float, Q16.16
    fixed-point, Q15 angle quantisation, varint zigzag, overflow
    detection.
  - `test_player_history.cpp` — 128-sample ring buffer, midpoint
    interpolation, refusal to extrapolate backwards, rewind cap.
  - `test_lag_compensation.cpp` — `computeViewTime` formula, hitbox
    geometry parity with the renderer cube.
  - `test_client_prediction.cpp` — predict + reconcile no-op match,
    pending input replay, snap on big drift, ring overflow.
- New CMake option `TRUESHOT_BUILD_TESTS` (default OFF, ON in CI). Adds
  `gtest` to `vcpkg.json` and a `tests/` subdirectory.
- New CI step `ctest --output-on-failure --build-config <type>` in
  `build.yml`, run after the build on every OS × build type.
- **Server-side network simulator** for reproducing bad conditions on
  demand, without OS firewalls or `tc`/`clumsy`:
  - `Net::Server::NetSimSettings { lossProbability, baseDelayMs,
    jitterMs }` + `setNetSimSettings` API.
  - `Server::sendTo` drops packets with probability `P` and/or
    enqueues them with a release timestamp.
  - `Server::step` drains the deferred queue using `std::stable_partition`
    so packets keep their enqueue order at the same release time.
- `trueshot_server` CLI flags:
  - `--port <N>` (also accepts a bare positional first arg for
    backwards compat).
  - `--simulate-loss <0.0-1.0>`
  - `--simulate-delay <ms>` (0-5000)
  - `--simulate-jitter <ms>` (0-1000)
  - `--help`
- `docs/test/phase-1-lan-test-plan.md` — six scenarios × three OS
  pairs × ten-minute sessions, with explicit acceptance criteria
  (no crash, no permanent desync, bandwidth ceiling, hit
  registration).

### Added — Phase 1.9 (Network metrics + HUD)

- **`Net::NetMetrics`** — POD aggregating every netcode number the HUD
  can display: connection state, RTT, local/server tick, player id,
  cumulative packets/bytes, derived per-second bandwidth/snapshot rates,
  pending inputs, last reconciliation correction, remote player count.
- **`Net::NetMetricsSampler`** — single-frame stateful low-pass filter
  (EMA, α = 0.20) over the cumulative byte/snapshot counters, exposing
  smooth per-second derivatives. Snapshot counter primed via
  `noteSnapshotReceived` on each `popSnapshot`.
- **`Hud::drawNetPanel`** — top-right ImGui overlay, non-interactive,
  with glance-able colour coding:
  - RTT green/amber/red at 0-100 / 100-200 / >200 ms
  - Last correction green/amber/red at <2 cm / 2-50 cm / >50 cm
  - Connection state colour-coded (connected = green, failed = red, etc.)
- Toggle bound to **F2** (edge-triggered, independent of F1). The panel
  is suppressed in offline mode (no `NetMetrics` to read).
- `Application` aggregates metrics every frame from `NetworkClient`,
  `ClientPrediction`, and `RemotePlayerRegistry`; passes the snapshot
  to `Hud::render` via an optional pointer (null in offline).
- Controls help updated to mention F2.

### Added — Phase 1.8 (Lag compensation for shots)

- **`Net::LagCompensation`** + `Net::PlayerHistory` — per-player ring
  buffer (128 samples ≈ 1 s at 128 Hz) of position/yaw/pitch
  snapshots, indexed by server wall time.
- `Net::computeViewTime(tNow, clientPingMs)` formula: rewinds the
  world to `T_now - RTT/2 - kClientInterpDelay (100 ms)`.
- `PlayerHistory::sampleAt(tTarget)` linearly interpolates between the
  two enclosing samples in the ring buffer.
- `LagCompensation::raycast` returns the closest victim, walking every
  other player's rewound hitbox.
- Slab-based ray-vs-AABB hit test (`rayVsAabb`) against placeholder
  hitboxes (0.8 × 1.8 × 0.4 — matches the remote-player render cube).
- **Hard 200 ms rewind cap** (`kRewindCapSeconds`) — anti-cheat against
  the "fake high ping → free extended rewind window" exploit.
- `Server::handleFire` invoked on every `InputState` carrying
  `InputButton::Fire`, immediately after `applyInput` so the shooter's
  pose is canonical when we read the ray origin.
- `Server::recordSample` runs once per simulation tick after all inputs
  have been applied — every tick of every player is rewindable.
- `Server::forgetPlayer` cleans up history on disconnect.
- `m_LagCompHits` counter exposed via `Server::lagCompHits()` for the
  Phase 1.9 network HUD.
- Debug log line `[Server] LAG-COMP HIT shooter=X victim=Y dist=Zm
  ping=Wms` so we can validate hits end-to-end without a kill feed yet.
- `glm` added as a public dependency of the `trueshot_network`
  library (was previously only available via the main TrueShot
  target's link line — broke the standalone `trueshot_server` build).
- ADR-006 — Lag compensation algorithm + thresholds.

### Added — Phase 1.7 (Client prediction + server reconciliation)

- **`Network/NetSim.h`** — shared authoritative simulation step
  (`stepSim`) used bit-identically by client and server. Phase 1.7's
  minimal sim is 5 m/s flat-ground movement with hard input clamping;
  the full Source-style movement migrates here in Phase 2.
- `Server::applyInput` now delegates to `Net::stepSim` — no more
  duplicated formula between client and server.
- **`ClientPrediction`** (`include/net/client_prediction.h`,
  `src/client_prediction.cpp`) — owns:
  - the local `SimState` rendered as "where we think we are right now",
  - a 256-entry ring buffer of pending (sent-but-not-ack'd) inputs,
  - `predict(input)` that steps the local sim and records the input,
  - `reconcile(ackSeq, authoritative)` that drops ack'd inputs, snaps
    to server truth, replays the still-pending inputs.
- Staged correction policy: ignore < 2 cm, soft lerp (25 % per
  reconciliation) for 2-50 cm drifts, hard snap >= 50 cm.
- Real keyboard inputs (WASD, Space, Ctrl/C, Mouse1, Mouse2, R) now
  populate `InputState.moveForward/moveRight/buttons` every tick instead
  of zeros.
- Debug log line `NET rtt=Xms pending=N predPos=(...) lastCorr=Xm` in
  `Application::printDebugInfo` so we can observe prediction health.

### Added — Phase 1.6 (Remote player + interpolation)

- `RemotePlayer` with a 64-sample ring buffer (~500 ms of history at 128 Hz).
- `RemotePlayerRegistry` ingesting every `Snapshot` and excluding the local
  player (local player is rendered from prediction, Phase 1.7).
- Linear interpolation 100 ms behind the latest snapshot
  (`kInterpDelaySeconds = 0.100`).
- Freeze fallback on starvation rather than extrapolating into nonsense.
- `Renderer::drawRemotePlayers` placeholder cubes (real character models
  arrive in Phase 4).
- ADR 0004 — Snapshot interpolation design.

### Added — Phase 1.5 (Authoritative server + listen-server foundation)

- `Net::Server` class with `start` / `stop` / `step` API, 128 Hz fixed
  accumulator capped at 0.25 s.
- Per-peer `PlayerState` with `lastAckedSeq`.
- **Hard server-side input clamp** as the foundation for Phase 9's custom
  anti-cheat: `moveForward/moveRight ∈ [-1, 1]`, `yaw ∈ ±180°`,
  `pitch ∈ ±89°`.
- Per-peer `Snapshot` broadcast with personalized `ackSeq`.
- Standalone `trueshot_server` binary
  (`network_module/src/main_server.cpp`).
- `Net::Server` exposed as a library so a client can host in listen-server
  mode (full integration in Phase 2).
- Windows linker: `ws2_32`, `winmm` linked for ENet.
- ADR 0003 — Listen-server + input clamping.

### Added — Phase 1.4 (`NetworkClient` integrated into `Application`)

- `NetworkClient` (ENet, 2 channels: reliable + unreliable sequenced) with
  `initialize` / `shutdown` / `connectTo` / `disconnect` / `tick` /
  `sendInput` / `popSnapshot`.
- Metrics surface: `state`, `roundTripMs`, `packetsSent/Recv`,
  `bytesSent/Recv`, `localId`.
- `AppConfig` with `Offline` / `Client` modes, CLI parsing for
  `--offline` / `--server host[:port]` / `--help`.
- Network step integrated into the main loop with a 128 Hz input
  accumulator (sending one `ClientInput` per simulation tick regardless of
  frame rate).
- Shared `enet_initialize` refcount between `NetworkClient` and `Net::Server`
  (required for listen-server).

### Added — Phase 1.3 (Packet schemas)

- `PacketType` enum: `Handshake`, `HandshakeAck`, `Disconnect`, `Ping`,
  `Pong`, `ClientInput`, `Snapshot`, `Event`, `RPC`.
- `InputState`, `EntityState`, `Snapshot`, `Handshake` POD types.
- Protocol version (`kProtocolVersion = 1`) negotiated in handshake.
- `serialize` / `deserializeBody` for every packet type.

### Added — Phase 1.2 (Bitstream)

- Little-endian explicit primitives (`writeU8/16/32/64`,
  `readU8/16/32/64`).
- `writeFloat` / `readFloat` via `memcpy` bit-cast.
- `writeQ16_16` fixed-point positions (4 bytes, ±32 km, 1/65536 precision).
- `writeAngleQ15` quantised angles (2 bytes, 0.0055° resolution at 180°).
- `writeVec3Q` (12 bytes).
- `writeVarU32` / `writeVarI32` zigzag varints (1-5 bytes).
- `writeString` length-prefixed.
- All `read*` functions return `bool` (false on overflow).

### Added — Phase 1.1 (TickClock)

- `TickClock` with fixed-timestep accumulator at 128 Hz, capped at 0.25 s
  to prevent the spiral of death on long pauses.
- `Physics::FIXED_TIMESTEP` constant shared between client and server.

### Added — Phase 1.0

- ADR 0002 — Netcode architecture (tick rate, protocol, listen-server,
  lag compensation strategy, anti-cheat foundation).

### Added — CI/CD hardening

- Multi-OS GitHub Actions matrix: Windows + macOS + Linux × Debug + Release.
- `clang-format` pinned to `18.1.8` via pip (eliminates apt drift).
- `editorconfig-checker` installed via direct binary download (avoids ARM
  Linux pip wheel build failures).
- `.ecrc` with `IndentSize` disabled (avoids fighting clang-format
  alignment).
- All third-party actions bumped to Node 24 native.
- vcpkg cached via `actions/cache@v4` keyed on `hashFiles('vcpkg.json')`.
- Configure step retried via `nick-fields/retry@v3` to survive transient
  GitHub 504s.
- `seanmiddleditch/gha-setup-ninja@v5` (rolled back from broken v6 zip).
- 3-state vcpkg bootstrap (clone if absent, fetch if `.git` exists,
  `init + reset --hard FETCH_HEAD` if dir restored without `.git`).

### Changed

- Replaced all emojis in C++ string literals (`🔊🔫🔄👟⏹️🏠`) with bracketed
  tags (`[AUDIO]`, `[WEAPON]`, ...) — clang-format was tripping on the
  whitespace handling around multi-byte glyphs.
- Headers: include order strictly `glad` → `GLFW` → third-party →
  project → stdlib (enforced by `.clang-format` `IncludeCategories`).
- `Server.cpp` lives in the library only; `main_server.cpp` provides the
  executable entry point (avoids ODR violation from double-compiling).
- ENet types in public headers exposed as opaque `void*` with
  `asHost(void*)` / `asPeer(void*)` helpers in `.cpp` (avoids leaking
  `<enet/enet.h>` into every translation unit).

### Fixed

- Windows MSVC `/W4`: silenced C4100/C4458/C4267/C4244 to match GCC's
  default behaviour rather than diverging from the cross-platform style.
- macOS build: `glad` must come before any header that pulls
  `<OpenGL/gl.h>`.

## [v0.1.0] — Phase 0 baseline

Initial proprietary codebase. Single-player practice range with:

- C++17 + CMake 3.16+ + vcpkg manifest mode.
- OpenGL 3.3 Core renderer (GLFW + GLAD + GLM).
- Source-style movement: strafe-jumping, bunny-hopping, fixed-timestep
  physics, wall bounces, friction, air-control, crouch.
- Five weapons (Glock, Deagle, AK-47, M4A4, AWP) with recoil patterns,
  ADS, reload behaviour.
- Real ray-vs-AABB hit detection with location-based damage.
- Dear ImGui HUD (score, ammo, accuracy, speed, FPS, bhop combo, crouch
  state, hit markers).
- OpenAL-ready audio architecture.
- Modern CMake (single `CMakeLists.txt`, presets, warnings + optional
  `-Werror`).
- LICENSE, CONTRIBUTING, exhaustive 20-phase ROADMAP.
