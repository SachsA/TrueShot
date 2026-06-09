# Phase 1.10 — 1v1 LAN Test Plan

This document captures the test pass that closes Phase 1. The goal is
to **prove the netcode is shippable** before Phase 2 builds match
structure on top of it.

We have two kinds of validation:

1. **Automated** — GoogleTest unit tests under `tests/`, run by CI on
   Windows + macOS + Linux every push. They cover the math:
   determinism, bitstream round-trips, lag-comp formula, prediction
   reconciliation.
2. **Manual** — this document. Two machines on a LAN, three OS pairs,
   ten-minute sessions with the server's built-in network simulator
   stressing the netcode under adverse conditions.

## Toolchain

You need a TrueShot build on at least two of {Windows, macOS, Linux}.
The CI artefacts (Release builds, 14-day retention) are the easiest
source — grab them from the run page of the commit you're testing.

```text
TrueShot-Windows-<sha>/  # contains TrueShot.exe, trueshot_server.exe, shaders/
TrueShot-Linux-<sha>/    # contains TrueShot,     trueshot_server,     shaders/
TrueShot-macOS-<sha>/    # contains TrueShot,     trueshot_server,     shaders/
```

## Server simulator CLI

The dedicated server can replay bad-network conditions without touching
the OS firewall. From Phase 1.10 onward:

```bash
trueshot_server                                          # 7777, no simulation
trueshot_server --port 9000                              # custom port
trueshot_server --simulate-loss 0.05                     # 5% packet drop
trueshot_server --simulate-delay 50 --simulate-jitter 20 # +50ms ± 20ms
trueshot_server --simulate-loss 0.10 \
                --simulate-delay 80 --simulate-jitter 30 # combined
```

Caps (enforced at parse time):

- `--simulate-loss`: 0.0 to 1.0
- `--simulate-delay`: 0 to 5000 ms
- `--simulate-jitter`: 0 to 1000 ms

The server prints `[Server] NetSim ENABLED: ...` on startup whenever
any of these is non-zero.

## Test matrix

Run each session for **at least 10 minutes**. Open the F2 network
panel on both clients and confirm the numbers stay healthy.

### Scenarios

| ID  | Loss  | Delay  | Jitter | Expected behaviour |
|-----|-------|--------|--------|--------------------|
| S1  | 0%    | 0 ms   | 0 ms   | Baseline. RTT ≈ LAN ping. `lastCorr` < 2 cm. |
| S2  | 5%    | 0 ms   | 0 ms   | Dropped Snapshots invisible (interp covers). `pending` stable. |
| S3  | 0%    | 50 ms  | 20 ms  | RTT shows ≈ 100 ms (round-trip = 2× one-way). Movement still feels fine. |
| S4  | 5%    | 50 ms  | 20 ms  | "Realistic shaky Wi-Fi". `lastCorr` < 20 cm transient. |
| S5  | 10%   | 100 ms | 50 ms  | "Bad coffee shop". Visible interp; reconciliation still recovers. |
| S6  | 20%   | 0 ms   | 0 ms   | Stress test. Expect dropped shots, but no crash, no permanent desync. |

### OS pairs

Cross-OS guarantees catch endianness bugs, struct-padding bugs, and
clock-source bugs that single-OS testing misses. Run **all three**:

- [ ] **Windows host ↔ macOS client** — scenarios S1, S3, S5
- [ ] **Windows host ↔ Linux client** — scenarios S1, S3, S5
- [ ] **macOS host ↔ Linux client** — scenarios S1, S3, S5
- [ ] **Listen-server** (one machine hosts AND plays) — S1

For each session, record:

- OS + build SHA on each side
- F2 panel screenshot at minute 1, 5, 10
- Any visible glitches: rubber-banding, missed hits, disconnects
- `lastCorr` worst case observed
- Whether `[Server] LAG-COMP HIT` log lines fire when you score

## Acceptance criteria

Phase 1 ships only if **all** these hold across the matrix above:

1. **No crash** in 10 minutes of any scenario.
2. **No permanent desync**: `lastCorr` always recovers to < 5 cm within
   1 second of leaving stress conditions.
3. **Connection survives 10 % loss + 50 ms jitter** (scenario S5)
   without ENet dropping the peer.
4. **Bandwidth stays under 16 KB/s up + 16 KB/s down** per client at
   2 players. If higher, investigate before Phase 2 scales to 10.
5. **Hit registration fires** when the shooter visibly lines up on a
   remote player (server log line shows up within ~1 s).
6. **Cross-OS parity**: identical input on Windows → macOS produces
   the same observable behaviour as macOS → Windows.

## Known not-in-scope (Phase 2)

- Per-client `--simulate-*` flags (currently only server-side).
- Packet-loss reporting in the F2 panel (counted; not displayed yet).
- Disconnect-and-reconnect mid-session (`Net::Server::start` is
  one-shot per process for now).
- Hit damage application + kill feed — Phase 1.8 only LOGS the hit;
  the visible effect lands in Phase 2 with HP system.
- Automated end-to-end LAN run in CI — would require two runner
  containers on the same network; Phase 8 (infra) territory.

## Running locally on a single machine

If you only have one machine, you can still exercise everything:

```bash
# Terminal 1
trueshot_server --port 7777 --simulate-delay 30 --simulate-jitter 10

# Terminal 2
TrueShot --server 127.0.0.1:7777

# Terminal 3 (optional)
TrueShot --server 127.0.0.1:7777
```

This won't catch endianness bugs (same OS on both ends) but covers
everything else, including the simulator. Use it to bisect issues
before re-running the full matrix.
