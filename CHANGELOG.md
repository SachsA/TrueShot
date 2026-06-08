# Changelog

All notable changes to TrueShot are tracked here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html)
once a first tagged release ships (`v0.1.0`, expected end of Phase 1).

Phase numbering matches [ROADMAP.md](ROADMAP.md). Decisions are detailed
in [docs/adr/](docs/adr/).

## [Unreleased]

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
