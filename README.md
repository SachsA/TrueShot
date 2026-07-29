<div align="center">

# TrueShot

**A tactical 5v5 FPS built in C++ with OpenGL, GLFW and ENet.**

[![build](https://github.com/SachsA/TrueShot/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/SachsA/TrueShot/actions/workflows/build.yml)
[![lint](https://github.com/SachsA/TrueShot/actions/workflows/lint.yml/badge.svg?branch=main)](https://github.com/SachsA/TrueShot/actions/workflows/lint.yml)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)](https://en.cppreference.com/w/cpp/17)
[![CMake](https://img.shields.io/badge/CMake-3.16%2B-064F8C?logo=cmake&logoColor=white)](https://cmake.org/)
[![OpenGL](https://img.shields.io/badge/OpenGL-3.3%20Core-5586A4?logo=opengl&logoColor=white)](https://www.opengl.org/)
![Platforms](https://img.shields.io/badge/platforms-Windows%20%7C%20macOS%20%7C%20Linux-lightgrey)
[![License](https://img.shields.io/badge/license-Proprietary-red)](LICENSE)

</div>

---

## Table of contents

1. [About](#about)
2. [Features](#features)
3. [Project structure](#project-structure)
4. [Getting started](#getting-started)
5. [Build & run](#build--run)
6. [Controls](#controls)
7. [Architecture overview](#architecture-overview)
8. [Roadmap](#roadmap)
9. [Documentation](#documentation)
10. [Contributing](#contributing)
11. [Security](#security)
12. [Socials](#socials)
13. [License](#license)

---

## About

TrueShot is an in-development tactical first-person shooter inspired by the
movement and gunplay of competitive Source-engine titles. We're currently
mid-**Phase 1** (1v1 LAN netcode) — the build runs both as a single-player
practice range and as a networked client/server pair, sharing the same C++
codebase. The full 5v5 match flow lands in Phase 2.

This repository hosts:

- the **TrueShot** client (OpenGL 3.3 + GLFW + Dear ImGui)
- the **`trueshot_server`** standalone authoritative server (ENet, 128 Hz tick)
- a shared **network module** (`Net::Server`, `NetworkClient`, packet codec,
  `RemotePlayer` interpolation) reusable in listen-server mode

See [**docs/adr/0002-netcode-architecture.md**](docs/adr/0002-netcode-architecture.md)
for the netcode design decisions (128 Hz, server-authoritative, listen-server,
custom kernel anti-cheat in Phase 9).

## Features

- **Source-style movement** — strafe-jumping, bunny-hopping, fixed-timestep
  physics, wall bounces, friction, air-control and crouch (eye-height
  interp + reduced ground speed) modelled after CS-style values.
- **Weapon system** — five weapons (Glock, Deagle, AK-47, M4A4, AWP) with
  their own damage, recoil patterns, fire modes, ADS times and reload
  behaviour.
- **Real hit detection** — ray-vs-AABB raycasting against a `GameWorld` of
  scoring targets, with location-based damage (head / chest / legs).
- **Score & accuracy tracking** — kills, hits, shots fired, accuracy %.
- **HUD overlay** — Dear ImGui panels for score, ammo, accuracy, speed, FPS
  and bhop combo. Toggle with `F1`.
- **Audio system** — OpenAL-ready architecture with 3D sources, footsteps,
  weapon cues and reverb zones.
- **128 Hz authoritative netcode** — ENet over UDP with two channels
  (reliable + unreliable sequenced), Q16.16 fixed-point positions, Q15
  quantised angles, varint zigzag. Server clamps every input as the
  foundation for Phase 9's custom anti-cheat. See ADR 0002.
- **Snapshot interpolation** — remote players are rendered 100 ms behind the
  latest snapshot using a 64-sample ring buffer per entity, freezing on
  starvation rather than extrapolating into nonsense. See ADR 0004.
- **Client prediction + reconciliation** — the local player's movement is
  applied immediately (zero perceived input lag), pending inputs are kept
  in a 256-entry ring, and each Snapshot's `ackSeq` triggers a replay
  from the server's authoritative state. The simulation step is shared
  bit-for-bit between client and server via `Net::stepSim`. See ADR 0005.
- **Lag compensation** — the server keeps a 1 s ring buffer of every
  player's pose and, at fire-time, rewinds the world to
  `T_now - RTT/2 - 100 ms` (where the shooter actually saw their target)
  before resolving the ray-vs-AABB hit test. Rewinds beyond 200 ms are
  refused as a "lag-for-free-shots" anti-cheat measure. See ADR 0006.
- **In-game network panel** — toggle with `F2`: connection state, RTT,
  local/server tick, bandwidth (up + down, EMA-smoothed), snapshot rate,
  pending input count, last reconciliation correction, remote player
  count. Colour-coded for glance-ability (RTT/correction thresholds).
- **Listen-server ready** — the `Net::Server` is built as a library so a
  client can host locally in addition to running the standalone
  `trueshot_server` binary.
- **Multi-OS CI/CD** — every push runs on Windows, macOS and Linux in
  Debug + Release with `-Werror`, plus clang-format, EditorConfig and
  markdownlint and prettier gates. Builds are reproducible against a pinned
  `clang-format-18.1.8`.
- **Modern CMake** — single `CMakeLists.txt`, presets, warnings enabled,
  optional Werror.

## Project structure

Headers live in `include/<subsystem>/`, implementations mirror them in
`src/<subsystem>/`, and every project include is subsystem-prefixed
(`#include "game/player_controller.h"`). See
[ADR-007](docs/adr/0007-source-layout.md) for why.

```text
TrueShot/
├─ include/                     # public headers, grouped by subsystem
│   ├─ core/
│   │   └─ application.h        # top-level lifecycle (init / run / shutdown)
│   ├─ render/
│   │   ├─ renderer.h           # OpenGL 3.3 renderer
│   │   └─ shader.h
│   ├─ game/
│   │   ├─ game_world.h         # targets, score, hit registration
│   │   ├─ target.h             # AABB target + ray intersection
│   │   ├─ fps_camera.h         # yaw/pitch camera
│   │   ├─ player_controller.h  # CS-style movement
│   │   └─ physics_types.h      # tuneable physics constants
│   ├─ weapons/
│   │   ├─ weapon_system.h      # weapons, recoil, ADS, hit detection
│   │   └─ weapon_types.h
│   ├─ audio/
│   │   ├─ audio_system.h       # 3D audio, footsteps, reverb
│   │   └─ audio_types.h
│   ├─ ui/
│   │   └─ hud.h                # ImGui-based in-game overlay
│   └─ net/                     # CLIENT-side networking
│       ├─ tick_clock.h         # fixed 128 Hz accumulator
│       ├─ network_client.h     # ENet client socket + metrics
│       ├─ remote_player.h      # snapshot interpolation registry
│       ├─ client_prediction.h  # local prediction + reconciliation
│       └─ net_metrics.h        # F2 panel data + EMA sampler
├─ src/                         # mirrors include/, plus main.cpp at the root
├─ netcode/                     # SHARED protocol + authoritative server
│   ├─ include/netcode/
│   │   ├─ bitstream.h          # LE primitives, Q16.16, Q15 angles, varint
│   │   ├─ net_common.h         # protocol constants + POD types
│   │   ├─ packet_types.h       # PacketType enum + serialize/deserialize
│   │   ├─ net_sim.h            # stepSim — identical on client & server
│   │   ├─ lag_compensation.h   # rewind buffer + hit registration
│   │   └─ server.h             # Net::Server (standalone + listen)
│   └─ src/
│       ├─ server.cpp
│       ├─ lag_compensation.cpp
│       ├─ net_common.cpp
│       └─ main_server.cpp      # trueshot_server entry point
├─ tests/                       # GoogleTest suite (ctest)
├─ shaders/                     # GLSL (basic.vert / basic.frag)
├─ scripts/                     # dev entry points (run from anywhere)
│   ├─ run.sh / run.bat         # build & run, forwards args to the binary
│   └─ clean.sh / clean.bat     # clean: build / --deps / --all
├─ docs/
│   ├─ README.md               # documentation index — start here
│   ├─ adr/                    # architecture decision records (+ template)
│   └─ test/                   # manual test plans
├─ .github/
│   ├─ workflows/              # multi-OS CI (build + lint + clang-tidy)
│   ├─ dependabot.yml          # weekly grouped Action bumps
│   ├─ ISSUE_TEMPLATE/
│   └─ pull_request_template.md
├─ CMakeLists.txt
├─ CMakePresets.json
├─ vcpkg.json                   # manifest mode deps
├─ .gitattributes               # line endings: LF in repo, CRLF for .bat
├─ .gitmessage                  # commit-message template (see CONTRIBUTING)
├─ CHANGELOG.md
├─ CLAUDE.md                    # working agreement (constraints, doc contract)
├─ AGENTS.md                    # pointer to CLAUDE.md for non-Claude agents
├─ CONTRIBUTING.md
├─ SECURITY.md                  # how to report a vulnerability
├─ LICENSE
└─ README.md
```

**`include/net/` vs `netcode/`** — the distinction matters: `include/net/`
is client-only code (prediction, interpolation, the socket, the HUD
metrics), while `netcode/` is the library that the client _and_ the
dedicated server both link, so its contents must stay free of any
client-side dependency.

## Getting started

### Prerequisites

| Tool                                        | Minimum version | Notes                           |
| ------------------------------------------- | --------------- | ------------------------------- |
| C++ compiler                                | C++17           | MSVC 19.30+, Clang 12+, GCC 10+ |
| [CMake](https://cmake.org/)                 | 3.16            | 3.21+ recommended for presets   |
| [vcpkg](https://github.com/microsoft/vcpkg) | latest          | installs the native deps        |

The Windows-specific Visual Studio / MinGW setup steps are in
[CONTRIBUTING.md](CONTRIBUTING.md#windows-toolchain-setup).

### Install dependencies (vcpkg)

```bash
git clone https://github.com/microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh        # or .\vcpkg\bootstrap-vcpkg.bat on Windows
export VCPKG_ROOT="$PWD/vcpkg"    # set %VCPKG_ROOT% on Windows

"$VCPKG_ROOT/vcpkg" install glfw3 glm "glad[gl-api-33]" enet "imgui[glfw-binding,opengl3-binding]"
```

### Clone

```bash
git clone git@github.com:SachsA/TrueShot.git
cd TrueShot
```

## Build & run

### One-liner

```bash
# macOS / Linux
./scripts/run.sh
./scripts/run.sh --server 192.168.1.42   # args are forwarded to the binary
./scripts/run.sh --no-run                # configure + build only

# Windows
scripts\run.bat
scripts\run.bat --server 192.168.1.42
```

Both accept `--help`, and both work from any directory — they resolve the
repo root from their own path. Override `BUILD_TYPE`, `BUILD_DIR` or
`VCPKG_ROOT` in the environment to change what they do.

### CMake presets (recommended)

```bash
cmake --preset default        # configure (Release)
cmake --build --preset default
./build/default/bin/TrueShot
```

Other presets: `debug`, `strict` (Werror).

### Manual

```bash
cmake -S . -B build \
      -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
      -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/bin/TrueShot
```

The build copies `shaders/` next to the executable automatically.

### Run modes

The client supports three modes selectable on the command line:

```bash
# 1. Offline practice range (default — no network)
./build/bin/TrueShot

# 2. Networked client — connect to a remote server
./build/bin/TrueShot --server 192.168.1.42
./build/bin/TrueShot --server 192.168.1.42:7777    # custom port

# 3. Standalone authoritative server (no rendering, no window)
./build/bin/trueshot_server                         # binds on :7777
./build/bin/trueshot_server --port 9000             # custom port

# 3b. Server with simulated bad network (for testing — Phase 1.10)
./build/bin/trueshot_server --simulate-loss 0.05    # 5 % packet loss
./build/bin/trueshot_server --simulate-delay 50 \
                            --simulate-jitter 20    # +50 ms +/- 20 ms
./build/bin/trueshot_server --help                  # list every flag
```

A typical 1v1 LAN session uses one machine running `trueshot_server` and two
machines running `TrueShot --server <ip>`. Listen-server mode (one client
hosts and plays at the same time) is wired into the same `Net::Server`
library and lands fully in Phase 2.

### Cleaning

Three levels, cheapest first. Both scripts take the same flags and refuse
to run outside the repo root.

```bash
# macOS / Linux                     # Windows
./scripts/clean.sh                  scripts\clean.bat
./scripts/clean.sh --deps           scripts\clean.bat --deps
./scripts/clean.sh --all            scripts\clean.bat --all
./scripts/clean.sh --dry-run        scripts\clean.bat --dry-run
```

| Level       | Removes                                                                                                    | Rebuild cost                                |
| ----------- | ---------------------------------------------------------------------------------------------------------- | ------------------------------------------- |
| _(default)_ | `build/`, `out/`, `dist/`, `cmake-build-*/`, `CMakeCache.txt`, `CMakeFiles/`, `compile_commands.json`      | Seconds — deps are kept                     |
| `--deps`    | ...plus `vcpkg_installed/` and the in-tree `vcpkg/` clone                                                  | Minutes — vcpkg re-installs the manifest    |
| `--all`     | ...plus the global vcpkg cache (`~/.cache/vcpkg`, `%LOCALAPPDATA%\vcpkg`) and `$VCPKG_ROOT`'s scratch dirs | 10+ min — everything recompiles from source |

`--dry-run` lists what would go without deleting anything; `--yes` skips
the confirmation that `--all` asks for. Only paths covered by
`.gitignore` are ever touched, so `git status` stays clean.

## Controls

| Action                        | Key                                                     |
| ----------------------------- | ------------------------------------------------------- |
| Move                          | `W` `A` `S` `D`                                         |
| Jump / bunny-hop              | `Space`                                                 |
| Crouch                        | `Ctrl` / `C`                                            |
| Look                          | Mouse                                                   |
| Fire                          | `Mouse 1`                                               |
| Aim down sights               | `Mouse 2`                                               |
| Reload                        | `R`                                                     |
| Switch weapon                 | `1` Glock · `2` Deagle · `3` AK-47 · `4` M4A4 · `5` AWP |
| Master volume                 | `+` / `-`                                               |
| Toggle audio debug            | `M`                                                     |
| Toggle HUD                    | `F1`                                                    |
| Toggle network panel (online) | `F2`                                                    |
| Quit                          | `Esc`                                                   |

## Architecture overview

```text
┌──────────────────────────────────────────────────────────────┐
│                       Application                            │
│  Owns the window, callbacks, main loop and every subsystem.  │
├──────────────────────────────────────────────────────────────┤
│  FPSCamera    PlayerController    WeaponSystem    Hud        │
│  GameWorld    AudioSystem         Renderer                   │
├──────────────────────────────────────────────────────────────┤
│  NetworkClient        RemotePlayerRegistry      (client mode)│
└──────────────────────────────────────────────────────────────┘
        │                │                       │
        ▼                ▼                       ▼
   yaw/pitch       fixed-timestep         raycast vs targets
                   physics                score / accuracy

┌──────────────────────────────────────────────────────────────┐
│      trueshot_server  (or any client in listen-server mode)  │
│  Net::Server — 128 Hz authoritative tick, broadcasts         │
│  Snapshot to every peer with personalized ackSeq.            │
│  Hard-clamps every InputState (anti-cheat foundation).       │
└──────────────────────────────────────────────────────────────┘
```

- `Application` is the only owner of subsystem lifetimes; everything else
  takes raw pointers / references.
- `Renderer` only knows about the camera, the world, the weapon (for FOV),
  the player and (optionally) the `RemotePlayerRegistry` — it never reaches
  back into input, audio or globals.
- `WeaponSystem::fire()` performs a real ray-vs-AABB raycast against
  `GameWorld::raycastTargets()` and applies damage based on hit location.
- **Server-authoritative.** The client predicts movement locally (Phase 1.7),
  but the server is the source of truth. Every `InputState` is clamped on
  arrival, every position broadcast back is what actually happened on the
  server. This is the foundation Phase 9's custom kernel anti-cheat plugs
  into.
- **No console** in any code path — Windows/macOS/Linux only (Steam +
  Steam Deck via Proton/native). See [ROADMAP.md](ROADMAP.md) Phase 20.

## Roadmap

Phase 1 — Netcode jouable (1v1 LAN) — is **code-complete**. Sub-phases
1.0 through 1.10 are all landed: design doc, tick clock, bitstream,
packet types, NetworkClient, authoritative server, snapshot
interpolation, client prediction + reconciliation, lag compensation,
network HUD overlay, GoogleTest suite, and the server-side bad-network
simulator. The remaining work is the manual cross-OS LAN test pass
described in
[docs/test/phase-1-lan-test-plan.md](docs/test/phase-1-lan-test-plan.md);
once that's signed off, we move to Phase 2 (full match flow, HP,
rounds, voice chat).

For the **full exhaustive roadmap** — network, anti-cheat, maps, art
direction, audio production, backend, e-sport, legal, marketing — see
[**ROADMAP.md**](ROADMAP.md). Twenty phases, ~400 atomic tasks, with
execution mode (solo / freelance / team) and budget estimates for each.

Recent changes are tracked in [**CHANGELOG.md**](CHANGELOG.md).

## Documentation

[**docs/README.md**](docs/README.md) is the index for everything written
down about TrueShot — the seven Architecture Decision Records with their
status and a one-line summary each, the manual test plans, and the gaps
that aren't written yet.

|                                         |                                         |
| --------------------------------------- | --------------------------------------- |
| Why the architecture is what it is      | [docs/README.md](docs/README.md) → ADRs |
| What's planned, phase by phase          | [ROADMAP.md](ROADMAP.md)                |
| How to set up and contribute            | [CONTRIBUTING.md](CONTRIBUTING.md)      |
| Hard project constraints + doc contract | [CLAUDE.md](CLAUDE.md)                  |
| How to report a security issue          | [SECURITY.md](SECURITY.md)              |

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for development setup, the coding
style and the pull-request workflow.

## Security

Found a way to make the server accept something a legitimate client
couldn't send? That's the report we want most. **Don't open a public
issue** — see [SECURITY.md](SECURITY.md) for the private channel and
what's in scope.

## Socials

- X / Twitter — <https://x.com/TrueShotGame>
- YouTube — <https://www.youtube.com/channel/UC0cwNEc0hI77cCWwX7EaNTg>
- Twitch — <https://www.twitch.tv/trueshotgame>

## License

TrueShot is **proprietary software**. All rights reserved. See
[LICENSE](LICENSE) for the full terms — in short, you may not use, copy,
modify, redistribute or sell any part of this project without explicit
permission from the author.
