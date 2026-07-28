# CLAUDE.md — working agreement for this repo

This file is read automatically at the start of every session. It is the
standing contract for how work happens on TrueShot. Read it fully before
touching anything.

---

## 1. Project constraints (NEVER violate, NEVER ask again)

These were decided by Alex and are not up for renegotiation unless he
explicitly reopens them.

| Constraint       | Decision                                                                                                                                                                                                                          |
| ---------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Platforms**    | **Windows + macOS + Linux are MANDATORY at every single step.** Every feature must build and run on all three before it counts as done. Consoles (PS5/Xbox/Switch) are explicitly out of scope — "absolument pas dans les clous". |
| **Distribution** | **Steam only.** No Epic, no standalone launcher, no other store. Steam Deck (Linux) must be Verified at launch.                                                                                                                   |
| **Anti-cheat**   | **Custom kernel-based AC, built in-house** with devs Alex knows. NOT EAC, NOT BattlEye, NOT any licensed solution. "Je veux le notre, kernel based, le meilleur AC de l'histoire, même si ça prend 3 ans."                        |
| **Tick rate**    | **128 Hz** fixed. Pro standard (CS2 / FACEIT / Valorant).                                                                                                                                                                         |
| **Authority**    | **Server-authoritative from Phase 1.** The client predicts, the server decides. Every value read from a packet gets clamped.                                                                                                      |
| **Hosting**      | **Listen-server** supported alongside the standalone dedicated server. One shared simulation, two hosting models.                                                                                                                 |
| **Quality bar**  | Alex wants "du parfait et giga fiable". When offered a minimal vs. complete approach, default to **complete**.                                                                                                                    |

---

## 2. THE DOC CONTRACT — non-negotiable, applies to every change

**Alex should never have to ask "did you update the docs?" again.**
After _any_ code change, proactively audit and update **everything** that
could possibly be affected. Not just the obvious files — everything.

### Always check these

**Documentation**

- `README.md` — features list, current-phase status sentence, run modes,
  controls table, architecture diagram, project tree, links
- `ROADMAP.md` — tick `[x]` completed items, document deferred items with
  the phase they moved to, keep phase/sub-phase numbering collision-free
- `CHANGELOG.md` — new entry at the top of `[Unreleased]`, descending order
- `CONTRIBUTING.md` — dev workflow steps, commit-message rules, coding
  style, architecture rules, CI notes, PR checklist
- `docs/README.md` — **the canonical docs index.** Every new ADR gets a row
  in its table; every new file under `docs/` gets listed. The ADR list
  lives _here only_ — never duplicate it into `CONTRIBUTING.md`
- `docs/adr/*.md` — write a new ADR for any architectural decision, copying
  `docs/adr/0000-adr-template.md`; keep numbering sequential and never
  recycle numbers
- `docs/test/*.md` — test plans when testing surface changes
- `SECURITY.md` — scope/out-of-scope when the threat surface changes, and
  the "design notes for reviewers" claims when a defence moves
- `.github/pull_request_template.md` — checklist when new gates appear
- `.github/ISSUE_TEMPLATE/*` — when templates go stale
- `LICENSE` — if the licensing model ever changes

**Build config**

- `CMakeLists.txt` (root) — new `.cpp` files (in the right subsystem
  block, alphabetical), new options, new defines, new link libraries,
  the configure-summary `message(STATUS ...)` block
- `netcode/CMakeLists.txt` — same, for the shared library + server
- `tests/CMakeLists.txt` — new test files
- `CMakePresets.json` — new or changed presets
- `vcpkg.json` — new dependencies, baseline bumps, version-string

**Tooling config**

- `.clang-format` — include categories when new directory layouts appear
- `.clang-tidy` — enabled/disabled checks, with a comment explaining why
- `.ecrc` — EditorConfig-checker exclusions
- `.editorconfig` — per-extension whitespace rules
- `.markdownlint.json` — markdown rule tweaks

**CI**

- `.github/workflows/build.yml` — configure flags, new steps, matrix,
  artefact packaging
- `.github/workflows/lint.yml`
- `.github/workflows/clang-tidy.yml`
- `.github/dependabot.yml` — new ecosystems when new dependency managers
  appear (Actions are covered; vcpkg is unsupported and stays manual)
- Any workflow that _should_ exist but doesn't yet

**Runtime / scripts**

- `scripts/run.sh` / `scripts/run.bat` — new CLI flags, new env vars,
  new build steps
- `scripts/clean.sh` / `scripts/clean.bat` — every newly generated
  directory must be added to the clean target list, in both scripts
- Any new script goes in `scripts/`, never at the repo root, and gets
  `--help` plus a repo-root guard like its siblings
- `shaders/` — if the renderer's expectations change

**Repo hygiene**

- `.gitignore` — every newly generated directory or file extension
- `.gitattributes` — line-ending handling for cross-OS scripts
- `.gitmessage` — if the commit convention in §3 ever changes, this template
  and `CONTRIBUTING.md`'s "Commit messages" section must both follow
- Directory layout conventions (`include/` headers, `src/` impl, etc.)

### Cross-file consistency — this is where things silently rot

- **ADR numbers**: never recycle. The "already written" list and the
  "to write" list must not overlap.
- **Phase / sub-phase numbers**: no duplicates (we already shipped a bug
  where two sections were both `### 8.8`).
- **Pinned versions**: `clang-format` version, vcpkg baseline, gtest,
  GitHub Action versions — consistent everywhere they're mentioned.
- **CLI flags**: documented identically in README, ROADMAP, test plans,
  and the binary's own `--help` output.
- **Shared constants**: values duplicated across the client/server
  boundary (e.g. `kClientInterpDelaySeconds`) must match, and the
  duplication must be commented in both places.
- **Feature bullets vs. reality**: if README claims a feature, the code
  must actually do it.

### Files to CREATE when they become relevant

Don't wait to be asked:

- `AUTHORS.md` / `MAINTAINERS.md` — when Alex brings people on
- `.github/CODEOWNERS` — when there's a team (pointless while solo: GitHub
  won't request review from the PR author)
- `CODE_OF_CONDUCT.md` — when the repo takes outside contributions
- `.vscode/extensions.json` + `launch.json` — recommended extensions
  (clangd, CMake Tools) and debug configs for `TrueShot` +
  `trueshot_server`
- `docs/ops/*.md` — runbooks, from Phase 8 (dedicated servers) onward
- `docs/anticheat/*.md` — design notes, Phase 9
- `docs/test/*.md` — a new test plan per major phase
- `.github/workflows/release.yml` — Phase 16, Steam depot packaging

Already created (don't recreate, just maintain): `SECURITY.md`,
`.github/dependabot.yml`, `docs/README.md`,
`docs/adr/0000-adr-template.md`, `.gitmessage`.

**And anything else that logically follows from the change.** This list is
a floor, not a ceiling. If a change touches something not listed here,
update that too and add it to this list.

---

## 3. COMMIT MESSAGES — always provide one, unprompted

Every response that changes code ends with **one single-line command**
Alex can copy-paste in one go — `&&`-chained, no multi-line block:

```bash
git add -A && git commit -m "<type>(<scope>): <subject>" -m "- <bullet>" && git push
```

One `-m` per bullet. If there's no body, a single `-m` is enough.

**Keep messages SHORT.** Alex reads them, he doesn't want an essay.

- Subject line under 72 chars.
- Body: **3 bullets max**, one line each. Often no body at all is right.
- No prose paragraphs, no re-listing what the diff already shows.
- If the change genuinely needs a long explanation, that explanation
  belongs in an ADR — not in the commit message.

```text
<type>(<scope>): <subject under 72 chars>

- <one line, only if it adds something the subject doesn't>
- <max 3 bullets total>
```

Types: `feat`, `fix`, `docs`, `chore`, `refactor`, `test`, `build`, `perf`,
`ci`, `style`.

Scopes in use: `net`, `hud`, `render`, `physics`, `weapons`, `audio`,
`cmake`, `ci`, `lint`, `docs`, `tests`.

Good:

```text
fix(net): clamp rewind window to 200 ms

- Anti-cheat: fake high ping was buying a wider rewind. See ADR-006.
```

Bad: anything with a paragraph, a bulleted list of every touched file,
or a recap of the implementation.

---

## 4. Definition of done — run this checklist before saying "finished"

1. `clang-format --dry-run --Werror --style=file $(git ls-files '*.cpp' '*.h' '*.hpp')`
   returns clean
2. `markdownlint-cli2` clean on all `.md`
3. Unit tests pass (`ctest`) — or the CI has been asked to confirm
4. `ROADMAP.md` reflects reality (ticked + deferred documented)
5. `CHANGELOG.md` has the entry
6. `README.md` matches what the code actually does now
7. A new ADR exists if the change was architectural
8. `CONTRIBUTING.md` mentions any new convention
9. `.gitignore` covers new build artefacts
10. `scripts/run.*` and `scripts/clean.*` still work with the new CLI
    surface and the new generated directories
11. Commit message written and handed to Alex

---

## 5. Technical conventions

- **C++17**, no compiler extensions
- **Naming**: `PascalCase` types, `camelCase` functions/variables,
  `m_PascalCase` members, `k` prefix for file-scope constants
- **Files and directories**: `snake_case`, no exceptions
- **Layout** (ADR-007): headers in `include/<subsystem>/`, sources
  mirrored in `src/<subsystem>/`. Subsystems: `core`, `render`, `game`,
  `weapons`, `audio`, `ui`, `net`. Every project include is
  subsystem-prefixed — `#include "game/player_controller.h"`.
- **`include/net/` is client-only; `netcode/` is shared with the
  dedicated server.** `netcode/` must never gain a client-side
  dependency (no GLFW / ImGui / renderer), or `trueshot_server` stops
  building headless. Dependency is one-directional.
- **Headers**: `#pragma once`, forward declarations over heavy includes
- **Include order** (enforced by `.clang-format`): `glad` → `GLFW` →
  third-party → project → stdlib
- **Never** put `enet/enet.h` in a public header — use opaque `void*` with
  `asHost()` / `asPeer()` helpers in the `.cpp`
- **Endianness**: always explicit little-endian via the `Bitstream`
  helpers. Never `memcpy` a multi-byte primitive onto the wire.
- **Fixed timestep**: 128 Hz, `Physics::FIXED_TIMESTEP`. Don't duplicate.
- **Server authority**: clamp every packet value inside `Net::stepSim`
- **No `std::endl`** — use `'\n'` (endl forces a stream flush)
- **No emoji in C++ string literals** — clang-format mishandles the
  multi-byte whitespace. Use `[AUDIO]`, `[WEAPON]`, etc.
- **GL calls** stay inside `Renderer` and shader code only
- **Local player** renders from prediction; **remote players** render from
  100 ms snapshot interpolation. Never mix.

---

## 6. Current state

Phase 1 (netcode, 1v1 LAN) is **code-complete**: sub-phases 1.0 through
1.10 all landed. Remaining Phase 1 work is Alex's manual cross-OS LAN
test pass — see
[`docs/test/phase-1-lan-test-plan.md`](docs/test/phase-1-lan-test-plan.md).

Next up is **Phase 2** (full match flow: teams, rounds, HP, buy phase,
grenades, objectives, voice chat).

Deferred from Phase 1, tracked in `ROADMAP.md`:

- Camera driven by `ClientPrediction::state()` instead of
  `PlayerController` (Phase 1.7b)
- `clientPingMs` vs. measured-RTT cross-check (Phase 2 anti-cheat)
- `PlayerHit` event packet + kill feed (Phase 2, needs HP system)
- Scrolling jitter graph in the F2 panel (Phase 2)
- Client-side packet-loss measurement (Phase 2)
- Delta-compressed snapshots (`ROADMAP.md` §1.11, revisit when bandwidth
  actually hurts)

---

## 7. Communication style Alex wants

- Concise and direct. Cut words that don't change the meaning.
- French is fine; technical terms in English are fine.
- When there's a minimal vs. complete option, recommend complete.
- Show the reasoning behind non-obvious technical choices, briefly.
- Don't ask permission for things this file already authorises.
