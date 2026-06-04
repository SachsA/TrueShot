<!--
Thanks for the PR! Fill in this template so the reviewer has full context.
-->

## What & why

<!--
One or two paragraphs explaining the change and the problem it solves.
Reference the ROADMAP phase / task if applicable.
-->

## Type of change

- [ ] Bug fix (non-breaking change that fixes an issue)
- [ ] New feature (non-breaking change that adds functionality)
- [ ] Refactor (no behaviour change)
- [ ] Build / CI change
- [ ] Documentation
- [ ] Breaking change (requires migration notes)

## Roadmap

- Phase: <!-- e.g. 1, 2.5, 8.8 -->
- Task: <!-- short title of the task in ROADMAP.md -->

## How I tested

<!--
Spell it out: "Built on macOS arm64 in Debug, ran the game, fired
50 shots at targets, observed kill markers, accuracy in HUD increased."
"Spun up two clients on LAN and confirmed positions converge."
-->

## Checklist

- [ ] Builds clean under `cmake --preset strict` locally
- [ ] `clang-format-18 --dry-run --Werror` passes
- [ ] Tested on **Windows + macOS** at minimum (Linux verified by CI)
- [ ] No new global variables
- [ ] No new GL calls outside `Renderer` / shaders
- [ ] No `static` locals that should be instance members
- [ ] Public functions have a 1-line comment explaining intent
- [ ] README / ROADMAP updated if user-visible behaviour changed
- [ ] CI is green
