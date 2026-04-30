# Contributing to TrueShot

TrueShot is a proprietary project — see [LICENSE](LICENSE) — but contributors
who have been granted access can use this guide to set up their environment
and submit changes.

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
"$VCPKG_ROOT/vcpkg" install glfw3 glm "glad[gl-api-33]" enet
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

# 5. Commit & open a PR against main
```

## Coding style

- **Language:** C++17, no compiler extensions.
- **Headers:** `#pragma once`, forward declarations preferred over heavy
  includes in headers.
- **Naming:** `PascalCase` for types, `camelCase` for functions/variables,
  `m_PascalCase` for member fields, `k` prefix for file-scope constants
  (e.g. `kCubeVertices`).
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

- `Application` is the only place that owns subsystems.
- Per-subsystem dependencies go through setters (`setAudioSystem`,
  `setGameWorld`) so each system can be tested in isolation.
- The fixed timestep (64 Hz) lives in `PlayerController::update`. Don't
  duplicate it elsewhere.
- Anything time-based must accept a `deltaTime`; never assume 60 FPS.
- New gameplay entities (pickups, doors, etc.) should follow the same
  pattern as `Target` — a plain struct with a method-level update +
  a `GameWorld`-style owner.

## Pull-request checklist

Before opening a PR, please confirm:

- [ ] Builds clean under `cmake --preset strict` (no warnings).
- [ ] No new global variables.
- [ ] No GL calls outside `Renderer` / shaders.
- [ ] Headers use `#pragma once` and forward declarations where possible.
- [ ] Public functions have a 1-line comment explaining intent.
- [ ] Player-visible behaviour change is reflected in `README.md`
      (controls, features) when relevant.
- [ ] Updated the roadmap if you've closed an item.
