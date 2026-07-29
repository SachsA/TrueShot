# Changelog

All notable changes to TrueShot are tracked here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html)
once a first tagged release ships (`v0.1.0`, expected end of Phase 1).

Phase numbering matches [ROADMAP.md](ROADMAP.md). Decisions are detailed
in the ADRs indexed in [docs/README.md](docs/README.md).

## [Unreleased]

### Added — Markdown formatting is now enforced

- **`.prettierrc.json` + `.prettierignore` + a `prettier` job in `lint`.**
  Markdown had been reformatted by a manual prettier pass with no config,
  no ignore file and no CI gate — and `ROADMAP.md` didn't even match
  prettier's own output. Anyone running prettier locally would have
  produced a 200-line diff burying the real change. The version is pinned
  (`3.3.3`) for the same reason clang-format is: a minor bump reflows every
  table in the repo. Prettier is scoped to Markdown only; C++ stays with
  clang-format, YAML with yamllint.
- markdownlint and prettier are complementary and both green: the former
  checks rules, the latter checks layout.

### Changed — One source of truth for the agent contract

- **`AGENTS.md` is now a pointer to `CLAUDE.md`, not a copy of it.** It had
  been created as a near-duplicate (270 lines, identical but for the title
  and a trailing stub) — two copies of the same contract that would have
  drifted apart on the first edit to either. `CLAUDE.md` is canonical and
  says so; `AGENTS.md` exists purely so agents looking for that filename
  find their way there, and carries an explicit instruction not to expand
  it back into a copy.

### Added — Repository documentation & hygiene

Filling the gaps a mature repo is expected to have, before Phase 2 starts
adding surface area.

- **[`docs/README.md`](docs/README.md)** — the canonical documentation
  index. ADR table with number, title, status, phase and a one-line
  summary; test-plan table; an explicit "not written yet" section so the
  gaps are visible instead of forgotten. The ADR list previously lived
  duplicated in `CONTRIBUTING.md`; that copy is gone, so it can only rot
  in one place now.
- **[`docs/adr/0000-adr-template.md`](docs/adr/0000-adr-template.md)** —
  copy-paste ADR skeleton matching the structure of the existing seven,
  with the status vocabulary and the never-recycle-a-number rule.
- **[`SECURITY.md`](SECURITY.md)** — vulnerability reporting policy, with
  a threat model stated in game terms (a speedhack _is_ a security bug).
  Scope, out-of-scope, and the four defensive claims a reviewer should try
  to break: the clamp in `stepSim`, the 200 ms rewind cap, explicit
  little-endian wire encoding, and `netcode/`'s dependency isolation.
- **[`.github/dependabot.yml`](.github/dependabot.yml)** — one grouped PR
  every Monday for GitHub Actions versions. Twelve hand-pinned `@vN` tags
  across three workflows were rotting with nothing watching them.
- **[`.gitmessage`](.gitmessage)** — commit-message template encoding the
  Conventional Commits rules from `CLAUDE.md` §3. Enable per clone with
  `git config commit.template .gitmessage`.
- `CONTRIBUTING.md` gained a **Commit messages** section and a note that
  vcpkg baseline bumps stay manual because Dependabot can't see them.

### Fixed — Cross-file consistency rot

Found by an audit sweep, not by CI. All three are the exact failure modes
`CLAUDE.md` §2 warns about.

- **ADR number collision.** `ROADMAP.md` listed `ADR-007` as still to be
  written (render abstraction for Phase 4) while ADR-007 had already
  shipped as the source-layout decision. The "to write" list is renumbered
  008–016, and ADR-007 is now ticked in the delivered list. That section
  is also relabelled as roadmap _tracking_ and points at
  `docs/README.md` as the canonical index.
- **`.editorconfig` referenced `network_module/build`**, a path that
  stopped existing when the module was renamed to `netcode/`. `.ecrc` had
  been updated at the time; `.editorconfig` was missed. Also now covers
  `vcpkg/`, and carries a note to keep the list in sync with `.gitignore`
  and `.ecrc`.
- **`.ecrc` excluded `^docs/.*\.md$`** — the project's own documentation
  was invisible to the EditorConfig check. All ten files pass without the
  exclusion, so it's gone. `*.bat` stays excluded, and _why_ is now
  documented in `CONTRIBUTING.md` since JSON can't carry a comment.
- `docs/adr/0006-lag-compensation.md` referred to `LagCompensation.cpp`
  (pre-rename path) → `netcode/src/lag_compensation.cpp`.
- `.gitattributes` now pins `.gitmessage` to LF explicitly.

### Changed — Source layout (ADR-007)

Structural housekeeping done between Phase 1 and Phase 2, while the tree
was still small enough for it to be cheap. All moves via `git mv`, so
`git log --follow` still works per file.

- **`include/` and `src/` grouped by subsystem** — `core`, `render`,
  `game`, `weapons`, `audio`, `ui`, `net`. `src/main.cpp` stays at the
  root of `src/` as the entry point. Every project include is now
  subsystem-prefixed (`#include "game/player_controller.h"`).
- **`network_module/` → `netcode/`**, `Network/` include prefix →
  `netcode/`, and every `PascalCase.h` → `snake_case.h`
  (`Bitstream.h` → `bitstream.h`, `LagCompensation.h` →
  `lag_compensation.h`, ...). The repo now has exactly one file-naming
  convention. CMake followed: target `trueshot_network` →
  `trueshot_netcode`, option `TRUESHOT_BUILD_NETWORK` →
  `TRUESHOT_BUILD_NETCODE`.
- Documented the **`include/net/` vs `netcode/`** boundary explicitly:
  the former is client-only, the latter is linked by the client _and_
  `trueshot_server` and must stay free of client-side dependencies.
  Dependency is one-directional.
- Root `CMakeLists.txt` source list regrouped by subsystem with blank
  lines between blocks, so adding a file touches one block instead of
  churning the whole list.

### Removed — Dead ENet prototype

- `network_module/src/Client.cpp`, `ENetWrapper.h` and
  `ENetWrapper.cpp` deleted, along with the `trueshot_client` CMake
  target. They formed a closed loop — `Client.cpp` was `ENetWrapper`'s
  only consumer and vice versa — and had been superseded by
  `NetworkClient` back in Phase 1.4. Six CI matrix jobs stop building
  an unused binary.

### Changed — Scripts moved into `scripts/`

- `RunTrueShot.sh` → **`scripts/run.sh`**, `RunTrueShot.bat` →
  **`scripts/run.bat`** (via `git mv`, history preserved). The repo root
  was accumulating entry points and Phase 8 (deploy) and Phase 16 (Steam
  packaging) will add more.
- Both `run` scripts now resolve the repo root from their own path, so
  `cmake -S .` is correct regardless of the working directory.
- `run.sh` / `run.bat` gained `--help`, `--no-run` (configure + build
  without launching) and **argument pass-through** — `./scripts/run.sh
--server 192.168.1.42` forwards straight to the binary, which matters
  now that the client has a real CLI surface.
- `run.bat` now probes both the multi-config
  (`build\bin\<Config>\TrueShot.exe`) and single-config
  (`build\bin\TrueShot.exe`) output paths instead of relying on
  `errorlevel` from a failed launch.

### Added — Clean scripts

- **`scripts/clean.sh`** / **`scripts/clean.bat`** — cross-platform clean
  with three levels:
  - _(default)_ build artefacts: `build/`, `network_module/build/`,
    `out/`, `dist/`, `cmake-build-*/`, `CMakeCache.txt`, `CMakeFiles/`,
    `CMakeUserPresets.json`, `compile_commands.json`;
  - `--deps` — also `vcpkg_installed/` and the in-tree `vcpkg/` clone;
  - `--all` / `--nuke` — also the global vcpkg cache
    (`~/.cache/vcpkg`, `%LOCALAPPDATA%\vcpkg`) and `$VCPKG_ROOT`'s
    `buildtrees`/`packages`/`downloads` (never the clone itself).
- Both scripts support `--dry-run` (preview), `--yes` (skip the `--all`
  confirmation) and `--help`; both refuse to run unless
  `CMakeLists.txt` + `vcpkg.json` are present, so they can't be
  pointed at the wrong directory.
- Only `.gitignore`d paths are ever removed — `git status` stays clean
  after any level.
- `.gitignore`: added `dist/` (CI artefact staging) and `vcpkg/` (the
  clone CI bootstraps into the repo root), both previously untracked
  but not ignored. Also flagged the `*.obj` collision between compiled
  objects and Wavefront meshes for whoever hits it in Phase 3.
- **`.gitattributes`** — line-ending normalisation, which the repo needed
  as soon as it had scripts for all three platforms. Everything is stored
  LF; `.bat`/`.cmd`/`.ps1` are converted to CRLF on checkout (cmd.exe
  mis-parses parenthesised blocks with bare LF), `.sh` stays LF even on
  Windows (git bash and WSL choke on CR in a shebang). Binary asset
  extensions are marked so git never tries to diff or convert them.
  This also resolves the standing mismatch where `.editorconfig`
  declared `crlf` for `.bat` while both existing `.bat` files were
  stored LF.

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
