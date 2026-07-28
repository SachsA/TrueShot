# Security policy

TrueShot is a competitive FPS. Its threat model is not "someone reads our
data" — it's **"someone gains an unfair advantage in a match"**. A
speedhack is a security bug here. So is a crash that can be triggered by a
remote peer, because a server you can crash is a server you can deny to
everyone else.

Please read the reporting rules below before opening anything public.

## Supported versions

Pre-release. Only `main` is supported — there are no tagged releases and no
backports yet. This section gets a table once TrueShot ships on Steam.

## Reporting a vulnerability

**Do not open a public issue for a security bug.** A public exploit
report is a free copy of the exploit for everyone reading the repo.

Use one of these instead, in order of preference:

1. **GitHub private vulnerability reporting** — the *Report a
   vulnerability* button under the repository's **Security** tab. Private,
   threaded, and it keeps the report attached to the repo.
2. **Email** — <alexandre.sachs@outlook.fr>, subject prefixed
   `[TrueShot security]`.

Please include, as far as you can:

- What an attacker gains (unfair advantage / crash / remote code
  execution / information disclosure).
- Reproduction steps. A crafted packet capture, a minimal patch against
  the client, or a short script beats prose.
- Which build — commit SHA, OS, `Debug` or `Release`, and whether you were
  on a listen-server or the standalone `trueshot_server`.
- Whether you believe it is already exploitable in the wild.

### What to expect

Solo project, so no SLA is promised. Realistically:

- **Acknowledgement** within a few days.
- **Assessment** — whether it's in scope, and how bad — shortly after.
- **Fix** prioritised by severity. Anything that lets a client overrule
  the server goes to the top of the queue ahead of feature work.
- **Credit** in `CHANGELOG.md` if you want it, or none if you'd rather.

Please give a fix a reasonable window before disclosing publicly. If a
report goes unanswered for 90 days, consider yourself free to disclose.

## In scope

- **Server authority bypass** — any input, packet, or timing trick that
  makes the server accept something a legitimate client could not produce.
  This is the highest-severity class in the project. See
  [ADR-003](docs/adr/0003-listen-server-and-input-clamping.md).
- **Lag-compensation abuse** — anything that buys a wider rewind window
  than the shooter's real latency justifies, e.g. by lying about ping. See
  [ADR-006](docs/adr/0006-lag-compensation.md).
- **Remote crashes and hangs** — a malformed or hostile packet that takes
  down `trueshot_server` or a listen-server host. Malformed input must be
  rejected, never fatal.
- **Memory safety over the wire** — out-of-bounds reads or writes reachable
  from packet data. The `Bitstream` readers are bounds-checked by design;
  a way around that is a bug.
- **Information disclosure to a peer** — anything leaking state a player
  shouldn't have (positions behind walls, other players' inputs). Note
  that Phase 1 broadcasts full snapshots to every peer *by design* —
  server-side visibility culling is Phase 4 work, tracked in
  [`ROADMAP.md`](ROADMAP.md). Reporting it is welcome but it's a known
  limitation, not a new finding.
- **Build and supply chain** — a way to get code executed through the
  CMake configure step, a CI workflow, or the vcpkg manifest.

## Out of scope

- **Cheats requiring local privilege** — DLL injection, memory editing,
  hardware cheats. These are real and they matter, but they are the
  in-house kernel anti-cheat's problem (Phase 9), not a repo
  vulnerability. Server-side detection ideas are still welcome as normal
  feature issues.
- **Third-party vulnerabilities** — report those upstream (ENet, GLFW,
  GLM, Dear ImGui). Do tell us if TrueShot uses a vulnerable version, so
  the pin can be bumped.
- **Anything that needs a modified server binary.** If you control the
  server you already have authority; that's the architecture, not a bug.
- **Missing hardening with no demonstrated impact** — "this function
  doesn't validate X" without a path to an outcome. Useful as a normal
  issue or PR, not as a security report.
- **Denial of service by volume** — packet floods and bandwidth
  exhaustion. Mitigation belongs to the hosting layer (Phase 8), and
  reporting that UDP can be flooded isn't a finding.

## Design notes for reviewers

The defensive posture is deliberate and documented, which should save you
time deciding whether something is intended:

- **Every value read from a packet is clamped** inside `Net::stepSim`, so
  the clamp applies identically on client and server and there is exactly
  one place to audit — [ADR-003](docs/adr/0003-listen-server-and-input-clamping.md),
  [ADR-005](docs/adr/0005-client-prediction-and-shared-netsim.md).
- **The rewind window is hard-capped at 200 ms** regardless of what ping a
  client claims — [ADR-006](docs/adr/0006-lag-compensation.md).
- **Wire encoding is explicit little-endian** via `Bitstream`; multi-byte
  primitives are never `memcpy`'d onto the wire, so there is no
  endianness- or padding-dependent parsing to confuse.
- **`netcode/` never links a client-side dependency**, which keeps the
  server's attack surface free of GLFW, ImGui and the renderer —
  [ADR-007](docs/adr/0007-source-layout.md).

If you find a hole in one of those four claims, that is exactly the report
we want.
