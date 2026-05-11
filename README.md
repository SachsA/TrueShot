<div align="center">

# TrueShot

**A tactical 5v5 FPS built in C++ with OpenGL, GLFW and ENet.**

[![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)](https://en.cppreference.com/w/cpp/17)
[![CMake](https://img.shields.io/badge/CMake-3.16%2B-064F8C?logo=cmake&logoColor=white)](https://cmake.org/)
[![OpenGL](https://img.shields.io/badge/OpenGL-3.3%20Core-5586A4?logo=opengl&logoColor=white)](https://www.opengl.org/)
[![Platforms](https://img.shields.io/badge/platforms-Windows%20%7C%20macOS%20%7C%20Linux-lightgrey)]()
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
9. [Contributing](#contributing)
10. [Socials](#socials)
11. [License](#license)

---

## About

TrueShot is an in-development tactical first-person shooter inspired by the
movement and gunplay of competitive Source-engine titles. The current build is
a single-player practice range that exercises the core systems (movement,
weapons, audio, networking) before they are wired together into the full 5v5
match flow.

This repository hosts both the **client** (rendered with OpenGL 3.3 + GLFW)
and a separate **network module** (ENet-based authoritative server / client
prototype).

## Features

- **Source-style movement** — strafe-jumping, bunny-hopping, fixed 64-tick
  physics, wall bounces, friction and air-control modelled after CS-style
  values.
- **Weapon system** — five weapons (Glock, Deagle, AK-47, M4A4, AWP) with
  their own damage, recoil patterns, fire modes, ADS times and reload
  behaviour.
- **Real hit detection** — ray-vs-AABB raycasting against an `GameWorld` of
  scoring targets, with location-based damage (head / chest / legs).
- **Score & accuracy tracking** — kills, hits, shots fired, accuracy %.
- **HUD overlay** — Dear ImGui panels for score, ammo, accuracy, speed, FPS
  and bhop combo. Toggle with `F1`.
- **Audio system** — OpenAL-ready architecture with 3D sources, footsteps,
  weapon cues and reverb zones.
- **Network module** — ENet-based prototype with input snapshots, server
  reconciliation scaffolding and a tiny bit-stream codec.
- **Modern CMake** — single `CMakeLists.txt`, presets, warnings enabled,
  optional Werror.

## Project structure

```
TrueShot/
├─ include/                 # public headers (one .h per subsystem)
│   ├─ application.h        # top-level lifecycle (init / run / shutdown)
│   ├─ renderer.h           # OpenGL renderer
│   ├─ game_world.h         # targets, score, hit registration
│   ├─ target.h             # AABB target + ray intersection
│   ├─ fps_camera.h         # yaw/pitch camera
│   ├─ player_controller.h  # CS-style movement
│   ├─ physics_types.h      # tuneable physics constants
│   ├─ weapon_system.h      # weapons, recoil, ADS, hit detection
│   ├─ weapon_types.h
│   ├─ audio_system.h       # 3D audio, footsteps, reverb
│   ├─ audio_types.h
│   ├─ hud.h                # ImGui-based in-game overlay
│   └─ shader.h
├─ src/                     # implementations
├─ shaders/                 # GLSL (basic.vert / basic.frag)
├─ network_module/          # ENet client/server prototype + library
├─ CMakeLists.txt
├─ CMakePresets.json
├─ RunTrueShot.sh           # macOS / Linux build & run
├─ RunTrueShot.bat          # Windows build & run
├─ CONTRIBUTING.md
├─ LICENSE
└─ README.md
```

## Getting started

### Prerequisites

| Tool | Minimum version | Notes |
|---|---|---|
| C++ compiler | C++17 | MSVC 19.30+, Clang 12+, GCC 10+ |
| [CMake](https://cmake.org/) | 3.16 | 3.21+ recommended for presets |
| [vcpkg](https://github.com/microsoft/vcpkg) | latest | installs the native deps |

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
./RunTrueShot.sh

# Windows
RunTrueShot.bat
```

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

## Controls

| Action | Key |
|---|---|
| Move | `W` `A` `S` `D` |
| Jump / bunny-hop | `Space` |
| Look | Mouse |
| Fire | `Mouse 1` |
| Aim down sights | `Mouse 2` |
| Reload | `R` |
| Switch weapon | `1` Glock · `2` Deagle · `3` AK-47 · `4` M4A4 · `5` AWP |
| Master volume | `+` / `-` |
| Toggle audio debug | `M` |
| Toggle HUD | `F1` |
| Quit | `Esc` |

## Architecture overview

```
┌──────────────────────────────────────────────────────────────┐
│                       Application                            │
│  Owns the window, callbacks, main loop and every subsystem.  │
├──────────────────────────────────────────────────────────────┤
│  FPSCamera    PlayerController    WeaponSystem               │
│  GameWorld    AudioSystem         Renderer                   │
└──────────────────────────────────────────────────────────────┘
        │                │                  │
        ▼                ▼                  ▼
   yaw/pitch      64-tick physics    raycast vs targets
                                     score / accuracy
```

- `Application` is the only owner of subsystem lifetimes; everything else
  takes raw pointers / references.
- `Renderer` only knows about the camera, the world, the weapon (for FOV)
  and the player — it never reaches back into input, audio or globals.
- `WeaponSystem::fire()` performs a real ray-vs-AABB raycast against
  `GameWorld::raycastTargets()` and applies damage based on hit location.

## Roadmap

- [x] In-game HUD (Dear ImGui — score, ammo, accuracy, speed, FPS)
- [ ] Wire the ENet network module into the main game loop
- [ ] Lobby + match flow (5v5 round structure)
- [ ] Map loading (BSP / glTF), AI bots
- [ ] Proper asset pipeline (textures, view-models, sound banks)
- [ ] Replay & demo system

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for development setup, the coding
style and the pull-request workflow.

## Socials

- X / Twitter — <https://x.com/TrueShotGame>
- YouTube — <https://www.youtube.com/channel/UC0cwNEc0hI77cCWwX7EaNTg>
- Twitch — <https://www.twitch.tv/trueshotgame>

## License

TrueShot is **proprietary software**. All rights reserved. See
[LICENSE](LICENSE) for the full terms — in short, you may not use, copy,
modify, redistribute or sell any part of this project without explicit
permission from the author.
