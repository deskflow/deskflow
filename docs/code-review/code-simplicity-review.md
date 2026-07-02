# Code Simplicity Review — Fleet State Hub (mesh v2)

**Scope:** `refactor/fleet-state-hub` vs `master`, plus uncommitted P2–P5 working-tree changes  
**Reviewed:** Production and test code under `src/` (docs excluded from removal recommendations)  
**Date:** 2026-07-02

---

## Simplification Analysis

### Core Purpose

Make the elected server the single source of truth for fleet topology, cursor host, and keyboard routing; replicate that snapshot to clients via mesh v2 `fleet` messages; let Server, clients, GUI, login bridge, watchdog, and vhid-bridge consume the snapshot instead of ad-hoc peer lists and legacy mesh side-channels.

The implementation largely achieves this. Complexity comes from **transitional v1/v2 dual paths**, **parallel data models for the same topology**, and **three independent localhost status pollers** parsing the same JSON line.

---

### Unnecessary Complexity Found

#### 1. Triple localhost status polling (Critical)

| Location | Mechanism |
|----------|-----------|
| `src/lib/gui/CoordinationStatus.cpp:89–138` | Async `QTcpSocket`, manual JSON parse, `formatFleetGraph` |
| `src/lib/coordination/CoordinationLocalStatus.cpp:55–87` | Sync `QTcpSocket`, manual JSON parse, host extraction |
| `src/apps/deskflow-vhid-bridge/deskflow-vhid-bridge.cpp:330–430` | Raw TCP + **string-scraping JSON parser** (`json_quoted_value`) |

All three send `{"t":"status"}\n` to `127.0.0.1:<coord-port>` and parse the same reply shape. The vhid-bridge parser is especially fragile (regex-free substring walks, no schema validation) and will drift when status JSON evolves.

**Suggested simplification:** One shared fetch layer. Qt consumers (`CoordinationStatus`, `LoginBridgeManager`, `MSWindowsWatchdog`) should call `pollLocalFleetStatus` (or a thin async wrapper). For vhid-bridge, extract a **non-Qt** status parser (or a minimal JSON helper) into `src/lib/coordination/` and reuse the same field mapping as `CoordinationLocalStatus.cpp`.

**Estimated LOC reduction:** ~90–120

---

#### 2. Incomplete v1/v2 dual paths — plan P4.8 not landed (Critical)

Plan phase P4.8 explicitly calls for deprecating `m_fleetCursorHost`, standalone mesh `cursor`, and dual relay branching. Uncommitted code still maintains:

- **Parallel cursor channels:** `Coordinator::broadcastCursor` + heartbeat rebroadcast (`Coordinator.cpp:327–353`, `811–841`) alongside mesh v2 `updateCursorHost` / `fleet` fragments.
- **Dual keyboard wire formats:** v1 `keyfwd` → elected server vs v2 `key` → cursor host (`Coordinator.cpp:529–534`, `600–622`).
- **Dual client relay gating:** v1 `ElectionState::cursorHere()` vs v2 `KeyboardRouter` + `fleetState.cursorHost` (`relayPassThroughLocal`, `651–677`).
- **Cached cursor host:** `m_fleetCursorHost` / `m_cursorSeq` shadow `m_fleetState.cursorHost` / `m_fleetState.seq`.

This is justified **during** migration but is the largest ongoing complexity tax (~150–200 LOC + branch surface). It should be tracked as **time-boxed debt**, not permanent architecture.

**Suggested simplification:** Finish P4.8/P6 cutover behind `meshVersion >= 2` guard, then delete v1 branches in one pass rather than accumulating more v1-aware call sites.

**Estimated LOC reduction (post-cutover):** ~150–200

---

#### 3. `FleetLink` vs `TopologyLink` duplicate adjacency types (Important)

| Type | Package | Direction representation |
|------|---------|--------------------------|
| `FleetLink` | `coordination/FleetState.h:24–29` | `std::string direction` ("left", "right", …) |
| `TopologyLink` | `server/TopologyLink.h:16–21` | `Direction` enum |

`ServerApp::applyFleetTopologyFromSnapshot` (`ServerApp.cpp:726–758`) converts string → enum in a loop. Server already understands `Direction` via `Config`.

**Suggested simplification:** Use one link struct end-to-end. Prefer `Direction` in fleet wire decode (map once in `CoordinationProtocol.cpp`) and drop `TopologyLink`, **or** typedef `TopologyLink` as `FleetLink` with a single conversion helper used at the Server boundary only.

**Estimated LOC reduction:** ~35–50

---

#### 4. `FleetFragment` duplicates `FleetState` (Important)

`FleetState.h:38–59` defines two nearly identical structs differing only in naming intent (snapshot vs wire fragment). `fleetFragmentFromMessage` (`CoordinationProtocol.cpp:328–331`) is a one-line alias.

Merge logic copies all fields wholesale (`FleetStateMerge.cpp:78–84`). The fragment type adds a type alias and conversion surface without distinct behavior.

**Suggested simplification:** Use `FleetState` (or a single `FleetSnapshot`) for both merge target and publish payload; pass partial updates as optional fields or a dedicated `FleetPatch` only if partial updates are actually needed (they are not today — server always sends full fragment).

**Estimated LOC reduction:** ~25–40

---

#### 5. `CoordinationStatus` does not reuse `CoordinationLocalStatus` (Important)

`CoordinationStatus::poll()` reimplements connect → write status → read line → parse JSON. `CoordinationLocalStatus::pollLocalFleetStatus` already does the same synchronously.

GUI polling could call `pollLocalFleetStatus` on a thread pool / `QtConcurrent` and emit results, keeping JSON field knowledge in one module.

**Estimated LOC reduction:** ~40–55

---

#### 6. Peer send loops still duplicated (Important)

`sendFleetLineToPeers` (`Coordinator.cpp:245–256`) centralizes fleet broadcast, but identical ip/lan fan-out appears in:

- `broadcastCursor` (348–352)
- `broadcastClaim` (769–777)
- Heartbeat cursor rebroadcast (832–839)

**Suggested simplification:** One `broadcastLineToPeers(const std::string &line)` private helper; all outbound mesh fan-out goes through it.

**Estimated LOC reduction:** ~30–40

---

#### 7. `friend class CoordinatorFleetPublishTests` for white-box testing (Important)

`CoordinatorFleetPublishTests.cpp:53–58` locks `coordinator.m_mutex` and calls `m_election.becameServer()` directly. This couples tests to private layout and makes refactors expensive.

**Suggested simplification:** Test via public API (`decide` is private, but role transitions can be driven through mesh `Claim` messages or a package-visible test hook). At minimum, expose a narrow `testOnlyArmServer()` in test builds.

---

#### 8. `ServerApp` topology extraction is heavy (Important)

`linksFromConfig` (`ServerApp.cpp:92–118`) samples **five fractional positions × four directions × every screen**, deduplicates with `std::set<std::tuple<…>>`. Correct for irregular edges, but expensive and hard to reason about.

**Suggested simplification:** If deskflow configs only define edges at `0.0` and `1.0`, sample those two points only. If fractional edges are rare, document the assumption and reduce samples to `{0.0f, 1.0f}` (~80% less work). Keep full sampling only if unit tests prove it is required.

**Estimated LOC reduction:** ~15–25 (or clarity gain with same LOC)

---

#### 9. Three `std::function` callbacks on `ServerApp` (Suggestion)

`setCursorBroadcastCallback`, `setFleetTopologyPublishCallback`, `setFleetSnapshotCallback` (`ServerApp.h:97–113`) wire AutoModeRunner → Coordinator. Works, but scatters coordination wiring across three injection points.

**Alternative:** Pass a single `ICoordinationFleet*` (or `Coordinator&`) into `ServerApp` for mesh v2 builds. Only worth doing if more callbacks appear; otherwise current pattern is acceptable.

---

#### 10. `RelayKeyPhase` + `Message::KeyPhase` + conversion helpers (Suggestion — acceptable)

`relayPhaseFromMessage` / `relayEventFromMessage` (`Coordinator.cpp:41–63`) exist because plan P4.1 deliberately decoupled server from protocol. The duplication is **intentional layering**, not YAGNI — keep unless protocol and relay merge later.

---

#### 11. `KeyboardRouter` as standalone 35-line module (Suggestion — keep)

`KeyboardRouter.cpp` is thin but **pure and well-tested** (`KeyboardRouterTests.cpp`). Extraction aids P4 matrix testing. Do not inline back into `Coordinator`.

---

#### 12. One-shot log flags (Suggestion)

Three booleans (`m_loggedKeyForward`, `m_loggedKeyForwardReceive`, `m_loggedRelayUnknownForward`) reset in four places in `updateKeyboardRelayForRole`. A single `enum class RelayLogOnce { Forward, Receive, UnknownForward }` with a small helper would reduce reset churn.

**Estimated LOC reduction:** ~10–15

---

#### 13. Redundant fleet events on ServerApp (Suggestion)

`registerFleetTopologyHandlers` registers **both** `CoordinationFleetStateChanged` and `CoordinationTopologyReady` with the **same** handler (`ServerApp.cpp:710–712`). Server only needs topology apply on any fleet change; `TopologyReady` is primarily for client pre-connect (`AutoModeRunner.cpp:232–239`). ServerApp can listen to `CoordinationFleetStateChanged` only.

**Estimated LOC reduction:** ~4–6

---

#### 14. `fleetFragmentFromMessage` one-liner (Suggestion)

`CoordinationProtocol.cpp:328–331` returns `message.fleet` — inline at the single call site in `Coordinator.cpp:317`.

**Estimated LOC reduction:** ~5

---

### Code to Remove (post-migration / safe now)

| Location | Reason | Est. LOC |
|----------|--------|----------|
| `deskflow-vhid-bridge.cpp:333–430` | Replace with shared status parser | 90 |
| `CoordinationProtocol.cpp:328–331` | Trivial wrapper | 5 |
| `ServerApp.cpp:712` | Duplicate event handler registration | 2 |
| `Coordinator.cpp` broadcastCursor heartbeat block | After v2-only cutover (P4.8) | 30 |
| `Coordinator.cpp` keyfwd branch | After v2-only cutover | 40 |
| `TopologyLink.h` + conversion loop | After FleetLink uses Direction | 25 |
| `FleetFragment` struct | After merge with FleetState | 20 |

---

### Simplification Recommendations (prioritized)

1. **Consolidate localhost status polling** (Critical)
   - Current: Three independent TCP+JSON implementations
   - Proposed: `CoordinationLocalStatus` for Qt; shared C parser for vhid-bridge
   - Impact: ~100 LOC, eliminates drift bug class

2. **Time-box and finish v1 path removal** (Critical)
   - Current: Parallel cursor, relay, and gating for mesh v1 and v2
   - Proposed: Single code path when `meshVersion >= 2`; delete v1 branches in P6
   - Impact: ~180 LOC, major clarity win

3. **Unify topology link types** (Important)
   - Current: String-direction fleet links → enum-direction server links
   - Proposed: Decode to `Direction` once in protocol layer
   - Impact: ~45 LOC, one mental model

4. **Merge FleetFragment into FleetState** (Important)
   - Current: Two identical structs + alias function
   - Proposed: One snapshot type for wire and merge
   - Impact: ~35 LOC

5. **Extract `broadcastLineToPeers`** (Important)
   - Current: Four copy-pasted ip/lan send loops in Coordinator
   - Proposed: Single helper used everywhere
   - Impact: ~35 LOC, fewer send bugs

6. **Rewire CoordinationStatus through CoordinationLocalStatus** (Important)
   - Current: Duplicate poll/parse in GUI module
   - Proposed: Shared fetch, GUI-only formatting stays local
   - Impact: ~50 LOC

7. **Reduce linksFromConfig sampling** (Important, verify first)
   - Current: 5 samples × 4 dirs × N screens
   - Proposed: 2 samples unless tests require 5
   - Impact: Runtime + readability

---

### YAGNI Violations

| Feature / abstraction | Why it violates YAGNI | What to do instead |
|-----------------------|----------------------|-------------------|
| vhid-bridge bespoke JSON scraper | Solves same problem as `CoordinationLocalStatus` without reuse | Shared parser module |
| `FleetFragment` separate from `FleetState` | No distinct lifecycle or fields | Single struct |
| `TopologyLink` separate from `FleetLink` | Conversion-only difference | One type with `Direction` |
| Standalone `cursor` mesh messages on v2 server heartbeat | Superseded by `fleet` cursor fields (plan P4.8) | Remove after cutover |
| `fleetFragmentFromMessage()` | No logic | Inline |
| Dual fleet events on ServerApp | Same handler for both | One event subscription |
| `friend CoordinatorFleetPublishTests` | Tests private members not behavior | Public/message-driven setup |

**Not YAGNI (keep):**

- `KeyboardRouter` — pure routing with matrix tests
- `RelayKeyEvent` — protocol decoupling for Server
- `CoordinationLocalStatus` — legitimate shared consumer API
- Queued switch / fleet topology on Server — required for offline-neighbor handoff
- `FleetStateMerge` + equality helpers — needed for change detection

---

### Test Code Notes

Tests are generally proportionate. Observations:

- **`ServerTests.cpp`** (+390 LOC committed, +178 uncommitted): High but tests real fleet-topology and queued-switch behavior. Keep; avoid further white-box tests against `m_active` / `m_clients` where public outcomes suffice.
- **`KeyboardRouterTests.cpp`**: Lean, valuable — model for pure logic tests.
- **`CoordinatorFleetPublishTests`**: Useful coverage but **`friend` + private mutex** is a maintainability smell — refactor before adding more coordinator tests.
- **`CoordinationLocalStatusTests` / `CoordinationStatusTests`**: Small, focused — good.

No test files flagged for removal.

---

### Final Assessment

| Metric | Value |
|--------|-------|
| **Production + test delta (approx.)** | ~2,800 LOC (branch + working tree, excl. docs) |
| **Total potential LOC reduction** | ~350–450 LOC now; ~180 more after v2-only cutover |
| **Reduction percentage** | ~12–18% immediate; ~25% after P6 |
| **Complexity score** | **Medium–High** (dual mesh versions, triple status poll, duplicate types) |
| **Recommended action** | **Proceed with simplifications** — architecture is sound; consolidate polling and types now, schedule v1 path deletion for P6 |

### Verdict

**Needs work** before calling the refactor "minimal." No rethink required — the fleet hub model is the right shape. Priority: (1) unify status polling, (2) merge duplicate types, (3) finish P4.8/P6 v1 removal so dual paths do not ossify.

---

## Issue Counts

| Severity | Count |
|----------|-------|
| Critical | 2 |
| Important | 7 |
| Suggestions | 6 |
