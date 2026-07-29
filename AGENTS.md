# AGENTS.md

**The working agreement for this repository lives in
[`CLAUDE.md`](CLAUDE.md). Read that file — all of it — before touching
anything.**

This file exists because different coding agents look for different
filenames (Codex and several others read `AGENTS.md`; Claude reads
`CLAUDE.md`). Rather than maintain two copies of a 250-line contract and
watch them drift apart, there is exactly **one** source of truth, and this
is a pointer to it.

`CLAUDE.md` covers:

| §   | What                                                                                                                              |
| --- | --------------------------------------------------------------------------------------------------------------------------------- |
| 1   | **Project constraints** — mandatory platforms, Steam-only, in-house kernel AC, 128 Hz, server authority. Never renegotiate these. |
| 2   | **The doc contract** — every file to audit after any change, plus the cross-file consistency rules.                               |
| 3   | **Commit messages** — Conventional Commits, kept short. See also [`.gitmessage`](.gitmessage).                                    |
| 4   | **Definition of done** — the checklist to run before claiming a task is finished.                                                 |
| 5   | **Technical conventions** — C++17, naming, source layout, the `include/net/` vs `netcode/` boundary.                              |
| 6   | **Current state** — which phase we're in and what's deferred.                                                                     |
| 7   | **Communication style.**                                                                                                          |

> **If you are an agent editing this repo:** do not copy `CLAUDE.md`'s
> contents into this file. If the contract changes, change `CLAUDE.md`.
> This page stays a pointer and nothing more.
