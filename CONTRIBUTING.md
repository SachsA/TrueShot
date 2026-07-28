# Contributing to TrueShot

TrueShot is a proprietary project — see [LICENSE](LICENSE) — but contributors
who have been granted access can use this guide to set up their environment
and submit changes.

> **Read [`CLAUDE.md`](CLAUDE.md) first.** It holds the project's hard
> constraints (mandatory platforms, Steam-only distribution, in-house
> kernel anti-cheat, 128 Hz server-authoritative netcode) and the
> documentation contract every change must satisfy. This file covers the
> mechanics; `CLAUDE.md` covers the rules.

## Table of contents

1. [Toolchain setup](#toolchain-setup)
2. [Development workflow](#development-workflow)
3. [Coding style](#coding-style)
4. [Architecture rules](#architecture-rules)
5. [Pull-request checklist](#pull-request-checklist)

---

## Toolchain setup

### Common

- C++17 compiler (MSVC 19.30+, Clang 12+, GCC 10+)
- [CMake](https://cmake.org/) ≥ 3.16 (3.21+ recommended for presets)
- [vcpkg](https://github.com/microsoft/vcpkg)
- A GPU + driver supporting OpenGL 3.3 Core

Install dependencies once:

```bash
"$VCPKG_ROOT/vcpkg" install glfw3 glm "glad[gl-api-33]" enet "imgui[glfw-binding,opengl3-binding]"
```

### Windows toolchain setup

1. **Visual Studio** — install [VS](https://visualstudio.microsoft.com/) with
   the *Desktop development with C++* workload. Restart your PC after
   installing.
2. **MinGW** (optional, for Make-based builds) — follow the
   [VS Code MinGW guide](https://code.visualstudio.com/docs/cpp/config-mingw)
   end to end, including the PATH steps. Restart all terminals afterwards.
3. **CMake** — install from [cmake.org](https://cmake.org/download/) (Windows
   x64 Installer) and tick *Add CMake to the system PATH*.
4. **vcpkg** — clone, bootstrap, then add the folder to your PATH:

   ```powershell
   git clone https://github.com/microsoft/vcpkg.git
   .\vcpkg\bootstrap-vcpkg.bat
   .\vcpkg\vcpkg integrate install
   ```

   Set `VCPKG_ROOT` in your user environment so the build scripts and
   presets find it automatically.

### macOS toolchain setup

```bash
brew install cmake
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
echo 'export VCPKG_ROOT="$HOME/vcpkg"' >> ~/.zshrc
echo 'export PATH="$VCPKG_ROOT:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

### Linux toolchain setup (Debian / Ubuntu)

```bash
sudo apt update
sudo apt install -y build-essential cmake git curl zip unzip pkg-config \
                    libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev \
                    libxcursor-dev libxi-dev libwayland-dev libxkbcommon-dev \
                    libasound2-dev
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
echo 'export VCPKG_ROOT="$HOME/vcpkg"' >> ~/.bashrc
source ~/.bashrc
```

Steam Deck (SteamOS) : utiliser `distrobox` ou un toolbox Arch pour le
dev confort ; le build natif fonctionne pareil que sur n'importe quel
Linux.

## Development workflow

```bash
# 1. Branch off main
git checkout -b feature/<short-name>

# 2. Build with presets
cmake --preset debug
cmake --build --preset debug

# 3. Run
./build/debug/bin/TrueShot

# 4. Lint with stricter warnings before pushing
cmake --preset strict
cmake --build --preset strict

# 5. Run the unit tests (Phase 1.10 onward)
cmake -S . -B build/tests \
      -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
      -DTRUESHOT_BUILD_TESTS=ON
cmake --build build/tests --parallel
ctest --test-dir build/tests --output-on-failure

# 6. Commit & open a PR against main
```

If a build starts behaving strangely — stale CMake cache, a dependency
that half-installed, a preset that won't reconfigure — clean and retry
before debugging further:

```bash
./scripts/clean.sh            # build artefacts only (fast)
./scripts/clean.sh --deps     # also drop vcpkg_installed/ and vcpkg/
./scripts/clean.sh --all      # also drop the global vcpkg cache
```

`scripts\clean.bat` is the Windows equivalent with identical flags. Add
`--dry-run` to preview. The scripts only remove `.gitignore`d paths, so
they can never eat your work, and they work from any directory.

## Coding style

- **Language:** C++17, no compiler extensions.
- **Headers:** `#pragma once`, forward declarations preferred over heavy
  includes in headers.
- **Naming:** `PascalCase` for types, `camelCase` for functions/variables,
  `m_PascalCase` for member fields, `k` prefix for file-scope constants
  (e.g. `kCubeVertices`).
- **File and directory names:** `snake_case`, everywhere, no exceptions.
  `lag_compensation.h`, not `LagCompensation.h`.
- **Includes:** always subsystem-prefixed —
  `#include "game/player_controller.h"`, never
  `#include "player_controller.h"`. See
  [ADR-007](docs/adr/0007-source-layout.md).
- **Const-correctness:** required. Mark accessors `const`.
- **Smart pointers:** `std::unique_ptr` for ownership, raw pointers for
  non-owning references.
- **OpenGL:** confine GL calls to `Renderer` and shader code. Other systems
  must stay GL-free.
- **Logging:** `std::cout` for debug lines, `std::cerr` for errors. Prefix
  with a subsystem tag, e.g. `[Renderer]`, `[Audio]`.
- **Avoid global state.** Subsystems are owned by `Application` and passed
  by pointer/reference where needed.

## Architecture rules

### Where code goes

Headers live in `include/<subsystem>/`, implementations mirror them in
`src/<subsystem>/`. Current subsystems: `core`, `render`, `game`,
`weapons`, `audio`, `ui`, `net`. A new subsystem is a new folder in
both trees plus a block in the root `CMakeLists.txt` — keep the blocks
alphabetical.

**`include/net/` vs `netcode/` — don't mix them up:**

| | Contents | May depend on |
|---|---|---|
| `include/net/` + `src/net/` | Client-only: prediction, interpolation, ENet client socket, HUD metrics | anything client-side |
| `netcode/` | Shared protocol + authoritative server, linked by the client *and* `trueshot_server` | **nothing client-side** — no GLFW, no ImGui, no renderer |

The dependency is one-directional: `include/net/` may include
`netcode/`, never the reverse. Breaking that makes `trueshot_server`
unbuildable on a headless box.

### Ownership and timing

- `Application` is the only place that owns subsystems.
- Per-subsystem dependencies go through setters (`setAudioSystem`,
  `setGameWorld`) so each system can be tested in isolation.
- The fixed simulation timestep is **128 Hz** (`Physics::FIXED_TIMESTEP`).
  Both the client input loop and `Net::Server::step` run on the same
  accumulator. Don't duplicate that timestep anywhere else.
- Anything time-based must accept a `deltaTime`; never assume 60 FPS.
- New gameplay entities (pickups, doors, etc.) should follow the same
  pattern as `Target` — a plain struct with a method-level update +
  a `GameWorld`-style owner.

### Netcode rules (Phase 1+)

- **Server is authoritative.** The client predicts movement locally
  (Phase 1.7) but never trusts its own state. Every value the server
  reads from a packet must be clamped against legitimate ranges before
  being fed to the simulation. See
  [ADR-003](docs/adr/0003-listen-server-and-input-clamping.md).
- **Endianness is explicit.** Never `memcpy` a multi-byte primitive onto
  the wire directly. Use the `Bitstream` helpers (`writeU16`, `writeQ16_16`,
  `writeAngleQ15`, etc.) — they encode little-endian regardless of host.
- **No `enet/enet.h` in public headers.** Use `void*` opaque pointers with
  `asHost(void*)` / `asPeer(void*)` helpers inside the `.cpp`. This keeps
  ENet out of every translation unit that includes the network headers.
- **One simulation, two hosting models.** `Net::Server` is a library that
  both `trueshot_server` (executable) and the future listen-server mode
  link against. Don't fork the server logic.
- **Local player is rendered from prediction, remote players from
  interpolation.** Snapshot interpolation runs 100 ms behind the latest
  snapshot. See
  [ADR-004](docs/adr/0004-snapshot-interpolation.md).

### Architecture Decision Records (ADRs)

Major architectural choices are recorded as ADRs under
[`docs/adr/`](docs/adr/). Read the relevant ADR before touching the
subsystem it covers; add a new ADR when you make a decision that future-
you (or someone else) would benefit from understanding the *why* behind.

Current ADRs:

- [ADR-001 — Render API](docs/adr/0001-render-api.md)
- [ADR-002 — Netcode architecture](docs/adr/0002-netcode-architecture.md)
- [ADR-003 — Listen-server & input clamping](docs/adr/0003-listen-server-and-input-clamping.md)
- [ADR-004 — Snapshot interpolation](docs/adr/0004-snapshot-interpolation.md)
- [ADR-005 — Shared NetSim + client prediction](docs/adr/0005-client-prediction-and-shared-netsim.md)
- [ADR-006 — Lag compensation](docs/adr/0006-lag-compensation.md)
- [ADR-007 — Source layout](docs/adr/0007-source-layout.md)

## Continuous Integration

Every push to `main` and every PR triggers two workflows on GitHub Actions:

| Workflow | What it does | Where to look |
|---|---|---|
| `build` | Compiles the project on **Linux + macOS + Windows**, both in `Debug` and `Release`, with `-Werror` on. Uploads `Release` artifacts for download. | `.github/workflows/build.yml` |
| `lint` | clang-format check, EditorConfig conformance, markdownlint, yamllint. Fast (~1 min). | `.github/workflows/lint.yml` |
| `clang-tidy` | Deep static analysis (runs weekly on Monday + on-demand). | `.github/workflows/clang-tidy.yml` |

**A PR is mergeable only when both `build` and `lint` are green.** This is
enforced by branch protection on `main` (set up in GitHub repo settings).

### Reproducing CI locally

If the CI fails, reproduce locally before pushing again. To eliminate any
chance of version drift between your machine and the CI, install the
**exact pinned version of clang-format** the CI uses:

```bash
pip install --user "clang-format==18.1.8"

# Format check (same command CI runs)
clang-format --dry-run --Werror --style=file $(git ls-files '*.cpp' '*.h' '*.hpp')

# Auto-fix formatting
clang-format -i $(git ls-files '*.cpp' '*.h' '*.hpp')

# Full strict build (will fail on any warning)
cmake --preset strict
cmake --build --preset strict

# Deep static analysis (slow)
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
clang-tidy-18 -p build $(git ls-files 'src/*.cpp')
```

### Updating dependencies

Native deps are declared in **`vcpkg.json`** (manifest mode). Adding one:

```bash
# 1. Edit vcpkg.json — add the package to the dependencies array.
# 2. Reconfigure locally — vcpkg installs it automatically.
cmake --preset default
# 3. Commit vcpkg.json. CI picks it up automatically.
```

When a new vcpkg release ships (every ~3 months), bump the
`builtin-baseline` SHA in `vcpkg.json` to the latest commit on
[microsoft/vcpkg main](https://github.com/microsoft/vcpkg/commits/master).

### Updating CI when the project grows

The CI is meant to evolve in lockstep with the project. Bookmarks:

- **Phase 1 (network)** → add a `network-test` job that boots a server,
  connects N fake clients, verifies state convergence.
- **Phase 9 (anti-cheat)** → add a `security` job (CodeQL, dependency
  scanning).
- **Phase 16 (Steam)** → add a `package-steam` job that produces a
  signed depot ready for `steamcmd app_build_*`.
- **Phase 17 (live ops)** → add a `release` workflow that tags a
  version, uploads to a Steam beta branch, posts to Discord.

Each phase's section in [ROADMAP.md](ROADMAP.md) carries a "CI updates
needed" reminder where relevant.

## Pull-request checklist

Before opening a PR, please confirm:

- [ ] The documentation contract in [`CLAUDE.md`](CLAUDE.md) §2 is
      satisfied — every affected doc, config, script and ignore file is
      updated, not just the code.
- [ ] Builds clean under `cmake --preset strict` (no warnings) on at
      least your dev OS — CI will run the other two.
- [ ] `clang-format --dry-run --Werror --style=file` passes on the
      changed files (use the pinned `clang-format==18.1.8`).
- [ ] No new global variables.
- [ ] No GL calls outside `Renderer` / shaders.
- [ ] No `enet/enet.h` leaked into a public header.
- [ ] Headers use `#pragma once` and forward declarations where possible.
- [ ] Public functions have a 1-line comment explaining intent.
- [ ] Player-visible behaviour change is reflected in `README.md`
      (controls, features) when relevant.
- [ ] Updated `ROADMAP.md` if you've closed an item, and `CHANGELOG.md`
      under `[Unreleased]`.
- [ ] If the change is architectural, added an ADR under `docs/adr/`.
