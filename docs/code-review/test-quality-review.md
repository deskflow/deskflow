# Test Quality Review — Fleet State Hub Refactor

**Scope:** Branch `refactor/fleet-state-hub` vs `master`, including uncommitted changes  
**Reviewed:** 2026-07-02  
**Stack:** C++ / Qt 6, QTest (`QTEST_MAIN`), CMake `create_test` + CTest  
**Test run:** All seven targeted suites pass via CTest from `build/src/unittests` with `QTEST_DISABLE_CRASH_HANDLER=1` on macOS.

---

## Test Quality Review

### Coverage Summary

- **Test run:** Pass (48 test cases across 7 suites, 0 failures)
- **Coverage:** Not instrumented in this review (no `ENABLE_COVERAGE` run); qualitative audit only
- **Files with tests:** 7/7 reviewed test targets exist and execute
- **Missing test files** (new implementation units without a corresponding test file):

| Implementation | Status |
|---|---|
| `src/lib/coordination/FleetStateMerge.cpp` | Covered by `FleetStateMergeTests.cpp` |
| `src/lib/coordination/KeyboardRouter.cpp` | Covered by `KeyboardRouterTests.cpp` |
| `src/lib/coordination/CoordinationLocalStatus.cpp` | Partial — `serverHostsFromStatus` only; `pollLocalFleetStatus` untested |
| `src/lib/gui/CoordinationStatus.cpp` | Partial — `formatFleetGraph` only; `CoordinationStatus::poll` untested |
| `src/lib/coordination/Coordinator.cpp` (fleet merge/broadcast path) | Partial — snapshot publish only via `CoordinatorFleetPublishTests.cpp` |
| `src/lib/server/TopologyLink.h` | Indirectly covered via `ServerTests.cpp` |
| `src/lib/server/Server.cpp` (fleet topology handoff) | Partial — one fleet-topology neighbor test + queued switch |

### Test Execution

```
Test project build/src/unittests
  CoordinationProtocolTests ........ PASS
  FleetStateMergeTests ............. PASS
  CoordinatorFleetPublishTests ..... PASS
  KeyboardRouterTests .............. PASS
  CoordinationLocalStatusTests ..... PASS
  CoordinationStatusTests .......... PASS
  ServerTests ...................... PASS
```

All suites registered in CTest when invoked from `build/src/unittests` (not the repo-root `build/` directory).

---

### Pure Logic Test Quality

#### `FleetStateMergeTests.cpp` — Pass with gaps

**Strengths**

- Covers the core merge contract: empty noop, full fragment replace, stale-seq rejection, equal-seq update, cursor ordering, and `topologyBecameReady`.
- Assertions verify observable state (`links`, `seq`, `cursorHost`) rather than internal helpers.
- Test names read as specifications (`staleFragmentIsIgnored`, `equalSeqReplacesStaleLinks`).

**Gaps**

- No test that `topologyBecameReady` stays `false` when topology was already present (regression guard for P3 pre-connect trigger).
- No test for `fragment.server.empty()` with non-empty links (early-return path in `applyServerFragment`).
- Peers vector replacement is never asserted independently.
- Plan task P2.5 ("non-server cannot override") lives in `Coordinator::handleFleetMessage`, not in the pure merge function — see Coordinator section below.

#### `CoordinationProtocolTests.cpp` — Pass

**Strengths**

- Excellent round-trip coverage for v1 and v2 messages: `hello`, `fleet`, `key`, `keyfwd`, `cursor`, legacy `claim`.
- Legacy compatibility explicitly tested (`decodesLegacyCoordinatorClaim`, string seq coercion).
- Malformed input returns `Invalid` — good failure-path coverage.
- `statusReplyIncludesFleetSnapshot` validates the P5 localhost status shape.

**Gaps**

- `fleetRoundTrip` does not assert peer round-trip (encode includes peers; decode assertion omits `peers.size()`).
- No decode tests for v2 `key` phase strings (only `keyfwd` phases in `keyFwdPhasesDecode`).
- No tests for partial/malformed fleet JSON bodies (missing `links`, empty `from`/`to`).

#### `KeyboardRouterTests.cpp` — Issues found

**Strengths**

- Pure-function tests with a shared `makeInput` helper — good pattern.
- Covers local cursor, remote forward, boot-grace unknown cursor, and two realistic 3-machine scenarios.
- Matrix-style tests (`matrix_serverLocal_slaveForwards`, `matrix_remoteCursor_serverForwards`) align with soak machine names from the plan.

**Gaps (plan mismatch)**

Plan P4.7 requires a **4×4 keyboard matrix** — 12 directed cases for `{hackintosh, macbookpro, tiny11}` × `{cursor local, cursor remote}` × `{type on each machine}`. Current tests cover roughly **4 of 12** directed scenarios:

| Plan scenario | Covered? |
|---|---|
| Cursor on server, type on slave | Yes (`matrix_serverLocal_slaveForwards`) |
| Cursor on server, type on server | Yes |
| Cursor on remote B, type on slave A | Partial (one A/B pair only) |
| Cursor on remote B, type on server | Yes (`matrix_remoteCursor_serverForwards`) |
| All permutations across 3 machines | No — only 2-machine pairs exercised |
| Unknown cursor after grace → forward with empty host | Yes |
| Unknown cursor during grace → local | Yes |
| Empty `cursorHost` with `cursorHostKnown=true` → local | No |
| Boot grace boundary at exactly `kCursorRelayBootGraceS` (0.3 s) | No |

This is the largest gap relative to plan acceptance criteria ("All 12 directed cases pass").

#### `CoordinationLocalStatusTests.cpp` — Issues found

**Strengths**

- Tests the pure `serverHostsFromStatus` helper with realistic JSON fixtures.
- Verifies self-exclusion and peer address/lan/name inclusion.

**Gaps**

- `pollLocalFleetStatus` — the TCP localhost poll used by LoginBridge and vhid-bridge (P5) — has **zero tests**.
- No deduplication test (`appendUnique` behavior when `server_ip` duplicates a peer IP).
- No empty/missing `fleet` object case.
- No case-insensitive self-name exclusion edge case (implementation uses `Qt::CaseInsensitive`).

---

### Integration Test Quality

#### `CoordinatorFleetPublishTests.cpp` — Issues found

**Strengths**

- Uses real `Coordinator` with `EventQueue`, `Arch`, and mesh port — closer to integration than pure mocks.
- Verifies public `fleetSnapshot()` after `updateCursorHost` and `publishFleetTopology`.
- `friend class` access is scoped and documented by necessity (election arming).

**Gaps**

- Only **2 tests** — both assert in-memory snapshot; neither verifies mesh broadcast, `CoordinationFleetStateChanged` event posting, or seq monotonicity across successive publishes.
- Plan P2.5: no test that **server ignores inbound `fleet` messages** (`handleFleetMessage` returns early when `role == Server`).
- No test that **client merges inbound fleet** and updates snapshot (the consumer path).
- No test for `meshVersion < 2` guard (v1 fallback to `broadcastCursor`).
- `armAsServer` bypasses normal election — acceptable for snapshot unit tests but leaves the publish/broadcast path largely unverified.

#### `ServerTests.cpp` — Pass with gaps

**Strengths**

- Fleet-specific additions are meaningful behavior tests, not tautologies:
  - `peekConfiguredNeighbor_usesFleetTopology` — fleet links override config when flag set.
  - `queuedSwitch_executesWhenNeighborConnects` — P3 queued handoff.
  - `adoptClient_resyncsEnterWhenActiveMatches` — P0 enter resync.
- Uses fixture pattern with real `Server` instances — consistent with existing suite.

**Gaps**

- Fleet topology test uses a single right-edge link; no multi-link or wrong-direction regression.
- No test that fleet mode **does not** fall back to `Config` links when fleet links are empty (P3 cutover flag semantics).
- No test for `injectForwardedKey` / keyboard routing at the server layer (P4.5 — may belong in a future server keyboard test).

---

### UI Test Quality

#### `CoordinationStatusTests.cpp` — Issues found

**Strengths**

- Tests the pure `formatFleetGraph` formatter with a realistic screens + links fixture.
- Empty fleet returns empty string — good edge case.

**Gaps**

- `CoordinationStatus::poll`, `start`, `stop`, and `online`/`offline` signal emission are **completely untested**.
- No test for screens-only fleet (links empty → `"alpha · beta"` without edges).
- No test for links with missing `dir` (formatter has a branch for dir-less edges).
- No test for malformed link objects (empty `from`/`to` skipped in production code).

For Qt networking components, the project pattern (see `NetworkMonitorTests`) suggests a `QTcpServer` stub on localhost would be appropriate.

---

### Pattern Compliance

| Pattern | Status | Notes |
|---|---|---|
| QTest `QObject` + `private Q_SLOTS` | Compliant | All suites follow project convention |
| `QTEST_MAIN` entry point | Compliant | |
| `initTestCase` / `cleanupTestCase` | Partial | Used in integration suites; pure logic suites omit (acceptable) |
| Descriptive test names | Compliant | Names read as specs throughout |
| Behavior-focused assertions | Compliant | No implementation-mirroring detected |
| Group organization | N/A | Flat slot lists; acceptable for small suites |
| Mocking / fakes | Partial | ServerTests uses `TestClientProxy`; Coordinator tests use friend access instead of fakes |

---

### Anti-Patterns Found

No tautological assertions, empty tests, or "mock the class under test" patterns were found.

| Location | Anti-pattern | Issue | Fix |
|---|---|---|---|
| `CoordinatorFleetPublishTests.cpp:53-58` | White-box friend access | Tests snapshot after manually mutating `m_fleetState` / election, not the real server promotion → publish pipeline | Add tests that drive `handleFleetMessage` on a client coordinator with injected messages; keep friend tests as supplementary |
| `KeyboardRouterTests.cpp` (suite) | Incomplete matrix vs plan | Plan P4.7 acceptance gate requires 12 directed cases; suite covers ~4 | Add parameterized test table enumerating all `{machine, cursorHost}` pairs from the plan |
| `CoordinationLocalStatusTests.cpp` (suite) | Missing async/network test | `pollLocalFleetStatus` is production-critical for P5 consumers but untested | Add test with `QTcpServer` returning a canned status JSON line |

---

### Recommendations

1. **Complete the P4.7 keyboard matrix** — Add a `QTest` data-driven table in `KeyboardRouterTests.cpp` with all 12 directed cases from the plan (3 machines × 4 cursor/type scenarios). This is the highest-impact gap blocking plan acceptance.

2. **Add Coordinator fleet merge integration tests** — New tests (extend `CoordinatorFleetPublishTests` or separate file):
   - Client receives `fleet` message → snapshot updates, `topologyBecameReady` fires.
   - Server ignores inbound `fleet` message (topology authority).
   - Verify seq increments and optional mesh send side effects via a test double for `CoordinationMesh`.

3. **Test `pollLocalFleetStatus`** — Minimal `QTcpServer` on ephemeral port; assert parsed `LocalStatusFleet` fields. Unblocks P5 consumer confidence without manual soak.

4. **Extend `CoordinationStatusTests`** — Mock localhost status server; verify `online` signal carries role, server short name, and formatted fleet graph; verify timeout → `offline`.

5. **Fill pure-function edge cases** — `topologyBecameReady` false when links already present; `formatFleetGraph` screens-only; boot grace at exactly 0.3 s; `serverHostsFromStatus` dedup.

---

### Verdict

**Fix 2 critical issues before merging** against plan acceptance criteria.

The pure-logic and protocol tests are solid and all pass. The refactor's highest-risk paths — **full keyboard routing matrix (P4.7)**, **Coordinator fleet authority (P2.5)**, and **P5 localhost polling** — are under-tested relative to the plan. Server handoff tests cover the P3 basics well. No anti-patterns or false-confidence tests were found; the gap is missing coverage, not bad coverage.

| Severity | Count |
|---|---|
| Critical | 2 |
| Important | 5 |
| Suggestions | 5 |
