# ADR-007 — Source layout: subsystem folders + one naming convention

- **Status:** Accepted
- **Date:** 2026-07 (between Phase 1 and Phase 2)
- **Phase:** Housekeeping, before Phase 2
- **Supersedes:** none
- **Superseded by:** none
- **Related:** [ADR-003 — Listen-server & input clamping](0003-listen-server-and-input-clamping.md)

## Context

At the end of Phase 1 the tree had three structural problems that were
cheap to fix then and would only get more expensive:

1. **`include/` was flat** — 13 headers at the top level plus a single
   `net/` subdirectory. Phase 2 alone (teams, rounds, HP, buy phase,
   grenades, objectives, voice chat, text chat) adds roughly 20 files,
   and Phase 6 (full UI/UX) adds as many again. A 50-header flat
   directory is unnavigable.
2. **Two naming conventions in one repo** — the game side used
   `snake_case` files under `include/` + `src/`, while the network
   library used `PascalCase` files under `network_module/include/Network/`.
   The container was `network_module` (snake) holding `Network` (Pascal).
3. **Dead code** — `Client.cpp` and `ENetWrapper.{h,cpp}` formed a
   closed loop: `Client.cpp` was the only consumer of `ENetWrapper`, and
   `ENetWrapper` was used by nothing else. Both had been superseded by
   `NetworkClient` in Phase 1.4, yet a `trueshot_client` binary was
   still built on all six CI matrix jobs.

There was also a naming collision waiting to happen: `include/net/`
(client-side networking) and `network_module/include/Network/` (shared
protocol) are conceptually different things with near-identical names.

## Decision

### 1. Group `include/` and `src/` by subsystem

```text
include/core/      application.h
include/render/    renderer.h, shader.h
include/game/      game_world.h, target.h, fps_camera.h,
                   player_controller.h, physics_types.h
include/weapons/   weapon_system.h, weapon_types.h
include/audio/     audio_system.h, audio_types.h
include/ui/        hud.h
include/net/       tick_clock.h, network_client.h, remote_player.h,
                   client_prediction.h, net_metrics.h
```

`src/` mirrors this exactly. `src/main.cpp` stays at the root of `src/`
because it is the entry point, not a subsystem.

Every project include is now subsystem-prefixed:

```cpp
#include "game/player_controller.h"   // not "player_controller.h"
#include "netcode/bitstream.h"        // not "Network/Bitstream.h"
```

This is slightly more typing but it makes the dependency direction
visible at the top of every file, which is what we actually want when
the file count grows.

### 2. One naming convention: `snake_case` everywhere

`network_module/` became `netcode/`, the `Network/` include prefix
became `netcode/`, and every `PascalCase.h` became `snake_case.h`:

| Before | After |
|---|---|
| `network_module/include/Network/Bitstream.h` | `netcode/include/netcode/bitstream.h` |
| `network_module/include/Network/NetCommon.h` | `netcode/include/netcode/net_common.h` |
| `network_module/include/Network/PacketTypes.h` | `netcode/include/netcode/packet_types.h` |
| `network_module/include/Network/NetSim.h` | `netcode/include/netcode/net_sim.h` |
| `network_module/include/Network/LagCompensation.h` | `netcode/include/netcode/lag_compensation.h` |
| `network_module/include/Network/Server.h` | `netcode/include/netcode/server.h` |

Types and functions keep their `PascalCase` / `camelCase` — only file
and directory names changed. The CMake target followed:
`trueshot_network` → `trueshot_netcode`, and the option
`TRUESHOT_BUILD_NETWORK` → `TRUESHOT_BUILD_NETCODE`.

`netcode` was chosen over `libnet` / `shared` / `protocol` because it's
the domain term already used throughout the ADRs, and because it reads
clearly next to `include/net/` without being confusable with it.

### 3. `include/net/` and `netcode/` are deliberately different things

- **`include/net/`** — client-only. Prediction, interpolation, the ENet
  client socket, HUD metrics. Free to depend on anything client-side.
- **`netcode/`** — linked by the client *and* the dedicated server.
  Must never gain a client-side dependency (no GLFW, no ImGui, no
  renderer). This is what makes `trueshot_server` buildable without a
  windowing system, and what will make listen-server mode work in
  Phase 2.

The rule is one-directional: `include/net/` may include `netcode/`,
never the reverse.

### 4. Delete the dead ENet prototype

`network_module/src/Client.cpp`, `ENetWrapper.h` and `ENetWrapper.cpp`
removed, along with the `trueshot_client` CMake target. The real client
has been `src/net/network_client.cpp` since Phase 1.4.

## Consequences

### Positive

- Adding a Phase 2 subsystem means adding a folder, not growing a flat
  list. Ownership is obvious from the path.
- One convention across the whole repo. No more guessing whether a
  header is `LagCompensation.h` or `lag_compensation.h`.
- Subsystem-prefixed includes make illegal dependencies visible in code
  review — `netcode/*.cpp` including `"ui/hud.h"` is now obviously
  wrong at a glance.
- Six CI jobs stop building an unused binary.
- `git mv` throughout, so `git log --follow` still works on every file.

### Negative

- Every `#include` in the project changed in one commit. Any in-flight
  branch will conflict on includes — rebase cost is real but mechanical
  (the mapping is a pure rename table).
- Muscle memory: `"hud.h"` is now `"ui/hud.h"`.

### Neutral

- `.clang-format`'s `IncludeCategories` regex for project headers
  already accepted subdirectories (widened when `include/net/` was
  introduced in Phase 1.6), so include ordering needed no change.
- `tests/` stays flat at five files. Group it if it passes ~15.

## Alternatives considered

### Leave `include/` flat and revisit later

Rejected on cost grounds. Reorganising 18 headers is one scripted pass
plus an include fix-up. Reorganising 50 headers mid-Phase-2, with
half-finished features in flight, is materially worse. The cheapest
moment to do this was before Phase 2 started, which is now.

### Adopt `PascalCase` everywhere instead of `snake_case`

Rejected. The game side is the larger half of the codebase and already
`snake_case`; converting it would have touched more files for the same
result. `snake_case` filenames with `PascalCase` types is also the more
common convention in modern C++ projects.

### Merge `netcode/` into `include/`+`src/` as another subsystem

Rejected. It must remain a separate library target so that
`trueshot_server` can link it without pulling in the renderer, the
audio system or ImGui. Keeping it a sibling directory makes that
boundary structural rather than a convention someone can accidentally
violate.

### Header-per-directory `all.h` umbrella headers

Considered and rejected — umbrella headers destroy incremental build
times by making every consumer depend on every header in the subsystem.
Explicit includes stay.
