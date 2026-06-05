# ADR-001 — Render API choice

- **Status:** Accepted (interim — revisit in Phase 4)
- **Date:** 2026-06-05
- **Phase:** 0 (Foundations) — interim decision until Phase 4 (Graphics)

## Context

The renderer needs to work on Windows + macOS + Linux. Apple deprecated
OpenGL above 4.1 in 2018 and removed Vulkan support entirely (you'd
have to go through MoltenVK).

## Decision

For Phase 0–3 (single-player + LAN multiplayer), we ship **OpenGL 3.3
Core** through GLAD + GLFW. This is what the practice range currently
uses and it works on all three OSes natively.

We will **re-evaluate in Phase 4 (Graphics)** when we need PBR,
deferred rendering, shadow maps and post-FX. The options at that point:

- Stay on OpenGL 4.1 (max on macOS).
- Move to Vulkan + MoltenVK on macOS.
- Adopt an abstraction layer like `bgfx` or `sokol_gfx` that targets
  GL + Vulkan + Metal under a single API.

The strong recommendation at that point will be the abstraction layer,
to avoid being painted into a corner by Apple's GL deprecation.

## Consequences

### Positive

- Simple, well-known API. Already working.
- Cross-platform without extra effort.
- Many tutorials and references available.

### Negative

- OpenGL is end-of-life on macOS. We will hit feature ceilings in
  Phase 4 (no compute shaders > 4.1, no descriptor sets, etc.).
- No multi-threaded command submission.

### Risk accepted

We acknowledge that Phase 4 will require a rendering rewrite. We are
keeping all GL calls inside `Renderer` (single file) precisely so that
this rewrite is bounded.
