# ADR-000 — Template (copy me, don't edit me)

<!-- markdownlint-disable-next-line MD028 -->

> **How to use this file.** Copy it to `NNNN-short-kebab-title.md` with the
> next free number, delete this blockquote and every HTML comment, then add
> a row to the table in [`docs/README.md`](../README.md). Never recycle a
> number — even for an ADR that gets superseded.

- **Status:** Proposed <!-- Proposed | Accepted | Superseded | Deprecated -->
- **Date:** YYYY-MM <!-- or YYYY-MM-DD if the exact day matters -->
- **Phase:** <!-- e.g. "2.3" or "Housekeeping, pre-Phase 3" -->
- **Supersedes:** none <!-- or a link: ADR-00X — Title -> 000X-slug.md -->
- **Superseded by:** none <!-- fill in later, when it happens -->
- **Related:** none <!-- ADRs a reader should have in mind first -->

## Context

What forced this decision? State the constraint, not the solution.

Good context is specific and falsifiable: numbers, platform limits, a bug
that actually happened, a requirement from
[`CLAUDE.md`](../../CLAUDE.md) §1. If a reader can't tell _why the obvious
thing doesn't work_, the context isn't done.

Keep the project's hard constraints in view — Windows + macOS + Linux at
every step, Steam-only distribution, 128 Hz, server-authoritative, custom
kernel AC. Several past decisions exist purely because of them.

## Decision

What we're doing. Present tense, active voice: "the server clamps every
input on arrival", not "inputs should probably be clamped".

Number the sub-decisions if there's more than one, so later ADRs and
commit messages can cite them precisely (`see ADR-00X §3`).

### 1. First sub-decision

### 2. Second sub-decision

Include the small amount of code or pseudo-code that makes the decision
concrete — a struct, a signature, the six steps of an algorithm. Not the
implementation; the shape.

```cpp
// Illustrative, not the real implementation.
```

Name the constants that encode the decision and say where they live, so
they're findable when someone wants to tune them.

## Consequences

### Positive

- What this buys us.

### Negative

- What it costs us. **Be honest here** — this section is the one future-you
  will actually reread, usually while paying one of these costs.

### Neutral

- Consequences that are neither, but that a reader would be surprised by.

## Alternatives considered

One `###` per alternative, each ending with why it lost. "Rejected"
without a reason is worse than not listing it at all — it hides the fact
that nobody thought it through.

### Alternative A

Rejected because …

### Alternative B

Considered seriously. Pros: … Cons: … Revisit if …
