# Plan technical review — FleetState hub mesh v2

**Artifact reviewed:** `docs/brainstorm/2026-07-02-fleet-state-hub-mesh-v2-refactor-brainstorm-doc.md`  
**Date:** 2026-07-02  
**Agents:** code-simplicity-review, vgv-review, plan-splitting

## Executive summary

The brainstorm correctly identifies **inconsistent partial state** (topology, cursor, election) as the root cause of edge slam and keyboard lag. The **FleetState hub** direction matches existing coordination patterns (`ElectionState` = pure logic, `Coordinator` = transport).

**Verdict: Not ready for `/build`.** Simplify scope and resolve two design tensions before writing the formal plan:

1. **Peer-merge topology** vs **server-authoritative broadcast** (simplicity review recommends broadcast for v1)
2. **Mesh v2 hard break** vs **additive v1 extension** (simplicity review recommends additive until soak proves break needed)

**Split: Recommended** — 7 PRs (P0–P6). Ship **P0 to `master` first** for immediate relief.

---

## Consolidated findings

### Critical (must address in plan)

| # | Issue | Source | Recommendation |
|---|-------|--------|----------------|
| C1 | No consumer API for FleetState | VGV | Add `FleetState.h`, `Coordinator::fleetSnapshot()`, events (`CoordinationFleetStateChanged`) |
| C2 | Layer leaks will repeat without neutral types | VGV | `RelayKeyEvent.h`; drop `CoordinationProtocol.h` from `Server.h` / `PrimaryClient.h` |
| C3 | No merge/router test plan | VGV | `FleetStateMergeTests`, v2 protocol tests, keyboard 4×4 matrix — gate each phase |
| C4 | Peer-merge is high complexity for v1 | Simplicity | Default to **server-published topology**; defer peer-merge unless multi-editor layout is required |

### Important

| # | Issue | Recommendation |
|---|-------|----------------|
| I1 | P5 bundles login + Mouser + HID + UAC before cutover | Soak gate on criteria **1–5** first; P5 consumers can follow cutover (v1.1) |
| I2 | Mesh v2 break + dual listener + kvmctl v2 upfront | Additive messages on 24851 first; hard break in P6 only |
| I3 | `ElectionState` should stay separate (Option A) | Gossip encodes election output; don't rewrite `ElectionStateTests` |
| I4 | `deskflow-vhid-bridge` can't link coordination lib | Localhost snapshot endpoint for login bridge (extend status poll) |
| I5 | Dual topology during P2–P3 | Plan explicit cutover: when `FleetState.links[]` replaces `Config` links |
| I6 | P0 re-scope gate missing | After P0 soak on master, reassess if full v2 is still required |

### Scope summary (plan-splitting)

| Metric | Value |
|--------|-------|
| Estimated LOC | ~3,500–5,500 |
| Layers | coordination, server, client, platform, gui, apps |
| Recommended PRs | **7** (P0 on master, P1–P6 on v2 branch) |
| Single PR risk | Unreviewable; no interim relief; all-or-nothing rollback |

---

## Recommended PR split (P0–P6)

| PR | Title | Branch | Depends on |
|----|-------|--------|------------|
| **P0** | `fix(fleet): adoptClient resync, pre-connect, switch-gating` | `master` | — |
| **P1** | `feat(coordination): mesh v2 shell + FleetState + merge` | v2 | — |
| **P2** | `feat(fleet): topology sync + GUI fleet graph` | v2 | P1 |
| **P3** | `feat(server): FleetState handoff integration` | v2 | P2 |
| **P4** | `feat(keyboard): unified cursor-host inject` | v2 | P1, P3 |
| **P5** | `feat(fleet): capability consumers (login/HID/UAC)` | v2 | P4 |
| **P6** | `feat(fleet): production cutover + kvmctl v2` | v2 | P1–P5 + soak |

**Soak gates:** after P3 (handoff), P4 (keyboard), P5 (full matrix), then P6.

**Optional:** Merge P2+P3 if boundary is painful; split P5 if > ~800 LOC.

---

## Simplification recommendations (apply to formal plan)

1. **Narrow v1 FleetState schema:** `links[]`, `cursor.{host,screen}`, `server`, `peers[]` only — drop `capabilities[]` and `cursor.pos` from v1.
2. **Server-authoritative topology** as default (override peer-merge brainstorm decision unless user reaffirms).
3. **Additive mesh messages** before protocol break; kvmctl v2 in P6 only.
4. **P0 → master → soak 1–2 weeks** before v2 branch diverges.
5. **Cross-link** `docs/plan/2026-06-30-fix-fleet-keyboard-handoff-still-failing-plan.md` — state what's superseded vs carried forward.
6. **Resolve open questions** in plan header (cursor.pos defer, hard cut, P0 on master).

---

## Module checklist (for `/plan`)

```
src/lib/coordination/
  FleetState.h
  FleetStateMerge.cpp          // pure, tested like ElectionState
  KeyboardRouter.h             // pure routing matrix
  RelayKeyEvent.h              // neutral types for server/
  CoordinationProtocol.cpp     // v2 codec
```

**Keep:** `ElectionState` separate; `AutoModeRunner` as glue; mouse on Deskflow TCP 24800.

---

## Decision needed from developer

**Peer-merge vs server-authoritative topology:** Simplicity review recommends changing the brainstorm's peer-merge decision for v1. Confirm in plan or keep peer-merge with explicit conflict test matrix.
