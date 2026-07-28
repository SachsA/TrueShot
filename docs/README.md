# TrueShot documentation

Everything written down about TrueShot lives here or is linked from here.
This page is the canonical index — **if you add a document under `docs/`,
add it to this page too.**

| Looking for… | Go to |
|---|---|
| What the project is, how to build and run it | [`README.md`](../README.md) |
| What's planned, phase by phase | [`ROADMAP.md`](../ROADMAP.md) |
| What changed and when | [`CHANGELOG.md`](../CHANGELOG.md) |
| How to work on the codebase | [`CONTRIBUTING.md`](../CONTRIBUTING.md) |
| How to report a security issue | [`SECURITY.md`](../SECURITY.md) |
| The working agreement for AI-assisted sessions | [`CLAUDE.md`](../CLAUDE.md) |
| **Why** the architecture is the way it is | [ADRs](#architecture-decision-records) below |
| How to test a phase before signing it off | [Test plans](#test-plans) below |

---

## Architecture Decision Records

An ADR records **one architectural decision**: the context that forced it,
the decision itself, the consequences accepted, and the alternatives
rejected. They are append-only history — an ADR is never edited to reflect
a later change of mind, it gets superseded by a new one.

Read the relevant ADR **before** touching the subsystem it covers.

| # | Title | Status | Phase | In one line |
|---|---|---|---|---|
| [001](adr/0001-render-api.md) | Render API choice | Accepted *(interim, revisit Phase 4)* | 0 | OpenGL 3.3 Core now, because it's the only API that ships on all three mandatory platforms without a rewrite. |
| [002](adr/0002-netcode-architecture.md) | Netcode architecture | Accepted | 1 | ENet/UDP, 128 Hz, server-authoritative, custom bit-packed wire format. The spine of everything. |
| [003](adr/0003-listen-server-and-input-clamping.md) | Listen-server & input clamping | Accepted | 1.5 | One simulation, two hosting models. Every packet value clamped on arrival — the anti-cheat safety net, from day one. |
| [004](adr/0004-snapshot-interpolation.md) | Snapshot interpolation | Accepted | 1.6 | Remote players render 100 ms in the past from a 64-sample ring. Freeze on starvation, never extrapolate. |
| [005](adr/0005-client-prediction-and-shared-netsim.md) | Shared `NetSim` + client prediction | Accepted | 1.7 | One `stepSim` compiled into both client and server, so prediction can't drift from authority. Reconcile by replay. |
| [006](adr/0006-lag-compensation.md) | Lag compensation | Accepted | 1.8 | Rewind the world to what the shooter saw, capped at 200 ms so a faked high ping can't buy a wider window. |
| [007](adr/0007-source-layout.md) | Source layout | Accepted | Housekeeping, pre-Phase 2 | Subsystem folders, `snake_case` everywhere, `include/net/` client-only vs `netcode/` shared. |

**Writing a new one:** copy
[`adr/0000-adr-template.md`](adr/0000-adr-template.md), take the next free
number, and add a row to the table above. Numbers are **never recycled**,
even if an ADR is later superseded.

**Status vocabulary**

- **Proposed** — written, not yet agreed.
- **Accepted** — in force. The code reflects it.
- **Superseded** — replaced by a later ADR, which is named in the header.
  The file stays; the history is the point.
- **Deprecated** — no longer in force and not replaced by anything.

> **Path notes.** ADRs **002, 003 and 005** were written before the
> layout reorganisation and refer throughout to the old paths
> (`network_module/`, the `Network/` include prefix, `PascalCase`
> filenames). Each carries a note at the top pointing at
> [ADR-007](adr/0007-source-layout.md). The decisions are unchanged; only
> the paths moved. ADRs 001, 004 and 006 needed no note — they reference
> few or no filenames.

---

## Test plans

Test plans cover what CI **cannot** check — anything needing real
hardware, multiple physical machines, or human judgement about game feel.

| Plan | Covers | Status |
|---|---|---|
| [Phase 1 — LAN test pass](test/phase-1-lan-test-plan.md) | 1v1 across Windows ↔ macOS ↔ Linux, plus listen-server on localhost. 6 scenarios × 3 OS pairs. | In progress — see [`ROADMAP.md`](../ROADMAP.md) §1.10 for the checkboxes |

One plan per major phase. Add the row above when you add the file.

---

## Not written yet

Tracked here so the gaps are visible rather than forgotten:

- `ops/` — dedicated-server runbooks (deploy, rotate, monitor, roll back).
  Due at Phase 8, when there is something to operate.
- `anticheat/` — design notes for the in-house kernel AC. Phase 9. Will
  be deliberately light on public detail.
- `test/phase-2-*.md` — Phase 2's test plan, when Phase 2 has a testing
  surface.
