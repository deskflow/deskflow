## VGV Code Review

### Summary

The fleet state hub mesh v2 refactor is well-structured for a staged rollout: mesh v2 is correctly gated on `Settings::Coordination::MeshVersion` (default `1`), legacy v1 paths remain intact for P6 cutover, and the core design — server-authoritative `FleetState`, pure `KeyboardRouter`, consumer read-only snapshots — matches the plan. Unit coverage for merge, protocol codec, keyboard routing, fleet publish, server topology handoff, and GUI formatting is solid.

However, **this is not ready to merge** until a handler-lifetime bug is fixed. `CoordinationKeyForward` handlers registered on the shared `EventQueue` system target are never removed when an epoch ends, while the coordinator continues to dispatch key events with `DeliverImmediately` from mesh threads. That creates a use-after-free window between epochs. There are also several mesh v2 correctness gaps around dual cursor signals and naming assumptions that will show up in soak testing.

Overall: **needs work** before merge; architecture direction is sound.

---

### 🔴 Critical — Must Fix Before Merge

- **`src/lib/deskflow/ServerApp.cpp:545` / `src/lib/deskflow/ClientApp.cpp:315` — `CoordinationKeyForward` handlers leak across epochs with dangling captures**
  - Why: Handlers are registered on `getEvents()->getSystemTarget()` with lambdas capturing raw `Server*` or `ClientApp*`. `stopServer()` / `stopClient()` never call `removeHandler(EventTypes::CoordinationKeyForward, …)`. The shared `EventQueue` in `AutoModeRunner` survives role flips. The coordinator posts key events with `Event::EventFlags::DeliverImmediately`, which synchronously dispatches from the mesh thread (`EventQueue::addEvent` line 194). Between epoch teardown and the next epoch's handler registration, a relayed key can invoke a stale lambda over freed memory.
  - Fix: Mirror `unregisterFleetTopologyHandlers()` — remove the handler in `ServerApp::mainLoop()` shutdown (after `loop()` returns) and in `ClientApp::stopClient()`. Prefer registering on a stable epoch target (e.g. the `App` instance) and calling `removeHandlers(app)` on teardown, or gate `DeliverImmediately` dispatch while `m_appRunning == false`.

- **`src/lib/coordination/Coordinator.cpp:513-524` — mesh v2 clients accept legacy `cursor` heartbeats without seq guard**
  - Why: `handleFleetMessage` correctly applies server fragments through `applyServerFragment` (seq-monotonic), but `handleCursorMessage` unconditionally overwrites `m_fleetState.cursorHost` / `cursorScreen` on non-server peers when `meshVersion >= 2`. The server worker still rebroadcasts legacy `cursor` messages every 3 s alongside `fleet` fragments (separate `m_cursorSeq` vs `m_fleetSeq`). A delayed or reordered cursor line can regress the fleet cursor after a newer fleet snapshot, breaking `KeyboardRouter` and client-side cursor-host inject.
  - Fix: On mesh v2 clients, either ignore `cursor` messages entirely (fleet fragment is authoritative), or track cursor sub-seq and only apply when `message.seq` advances a client-side cursor watermark tied to fleet epoch.

---

### 🟡 Important — Should Fix

- **`src/lib/coordination/KeyboardRouter.cpp:13-16` / `Coordinator.cpp:549-550` — `cursorHostIsLocal` compares `ComputerName` to deskflow screen names**
  - Why: `CoordinatorConfig::selfName` comes from `Settings::Core::ComputerName`, but `updateCursorHost` publishes the active **screen** name from `Server::SwitchToScreenInfo`. In layouts where screen aliases differ from computer names (multi-monitor configs, renamed screens), mesh v2 keyboard routing and client cursor-host inject will mis-route keys.
  - Fix: Document and enforce screen-name == self-name for coordinated fleets in v1, or resolve cursor host through fleet `screens[]` / canonical name mapping before comparing.

- **`src/lib/coordination/Coordinator.cpp:618-631` — unknown fleet cursor after boot grace silently drops keystrokes**
  - Why: When `cursorHostKnown == false` and grace expires, `routeKeyboard` returns `{Forward, {}}`. `sendKeyForward` then exits on `destination.empty()` with no fallback. Users cannot type until fleet cursor sync completes, with only a one-time log line.
  - Fix: Fall back to v1 server-address relay when `forwardHost` is empty, or extend grace / retry until first `fleet` fragment arrives; surface degraded state via status JSON for GUI.

- **`src/lib/coordination/Coordinator.cpp:84-86` — `peerMeshAddress` conflates cursor screen with server peer name**
  - Why: Fallback resolves `hostName == fleet.cursorScreen` via `matchConfigured(fleet.server)`. If cursor is on a non-server screen whose name differs from any peer entry, mesh address lookup fails even when the peer list contains the target machine under another name.
  - Fix: Walk `fleet.peers` by screen-to-host mapping from published topology, or include explicit mesh endpoint in fleet peer records.

- **`src/lib/deskflow/ClientApp.cpp:315-321` — `CoordinationKeyForward` handler registered once per client lifetime, not per epoch restart within same `ClientApp`**
  - Why: `startClient()` only registers when `m_clientScreen == nullptr`. If client restarts without full teardown in non-auto paths, handler may persist with stale state. Lower risk than cross-epoch UAF but inconsistent with handler hygiene.
  - Fix: Register in epoch setup, remove in `stopClient()` unconditionally.

- **Testing gap — no integration test for coordinator key relay dispatch lifecycle**
  - Why: `KeyboardRouterTests`, `CoordinatorFleetPublishTests`, and protocol tests cover units well, but nothing verifies that `handleKeyForwardMessage` → `CoordinationKeyForward` → inject works end-to-end, or that handlers are torn down cleanly on epoch exit. The UAF above would not be caught by current tests.
  - Fix: Add a coordinator test with mock event queue asserting handler removal, or an AutoModeRunner epoch-flip test that sends a `key` message during the inter-epoch gap.

- **`src/lib/coordination/CMakeLists.txt:47` — Qt dependency in coordination static library**
  - Why: Plan specified "no Qt in merge layer." `CoordinationProtocol.cpp` and `CoordinationLocalStatus.cpp` pull Qt JSON/network into `coordination`, which now propagates to `platform` (watchdog) and core binaries. This erodes layer separation and makes headless/testing builds heavier.
  - Fix: Accept for now if intentional, but split `coordination-protocol-qt` from pure merge/router headers, or migrate codec to a Qt-free JSON library for the merge layer.

---

### 🔵 Suggestions — Nice to Have

- **`src/apps/deskflow-vhid-bridge/deskflow-vhid-bridge.cpp:356-419` — ad-hoc JSON string parsing for status replies**
  - Suggestion: Reuse the same structured parsing as `CoordinationLocalStatus` / `protocol::decodeStatusReply` (or share a small Qt-free JSON helper) to avoid brittle `"name":"` substring walks breaking on field reordering or escaped strings.

- **`src/lib/gui/CoordinationStatus.cpp` vs `src/lib/coordination/CoordinationLocalStatus.cpp` — duplicated localhost status polling**
  - Suggestion: Have GUI call `pollLocalFleetStatus()` instead of reimplementing connect/write/read/parse, reducing drift between GUI, login bridge, watchdog, and vhid-bridge consumers.

- **`src/unittests/coordination/CoordinatorFleetPublishTests.cpp:53-58` — `friend class` + private mutex access**
  - Suggestion: Expose a test-only `CoordinatorTestHarness` or inject election state through a package-visible test seam rather than friending tests to production internals.

- **`src/lib/coordination/Coordinator.cpp:290-298` — `hello` handler does not validate peer identity**
  - Suggestion: After mesh v2 cutover (P6), consider replying only to known peers and using hello for version negotiation telemetry.

- **`src/lib/gui/core/CoreProcess.cpp:490-520` — `reloadServerConfig()` SIGHUP path**
  - Suggestion: Add a unit or integration test confirming SIGHUP triggers `ServerApp::reloadConfig()` → `publishFleetTopologyFromConfig()` so layout edits propagate fleet topology without full restart.

---

### Simplicity Assessment

- Lines that could be removed: ~80–120 (duplicate status polling in GUI; redundant cursor-channel updates on mesh v2 clients once fleet is authoritative)
- Unnecessary abstractions: None significant — `KeyboardRouter`, `FleetStateMerge`, `RelayKeyEvent`, and `TopologyLink` are appropriately scoped
- YAGNI violations: Dual cursor transport (legacy `cursor` + `fleet.cursor`) on mesh v2 clients adds complexity without clear v1 benefit once fleet fragments carry cursor; defer removal to P6 but stop applying legacy cursor on v2 clients now
- Complexity verdict: **Minor tweaks needed** — core factoring is right-sized; simplify by picking one cursor authority on mesh v2

---

### Testing Assessment

- New code with tests: ✅ FleetState merge, protocol v2 messages, KeyboardRouter matrix, Coordinator fleet publish, Server fleet topology / queued switch / resync enter, CoordinationLocalStatus host extraction, CoordinationStatus graph formatting
- Test quality: **Meaningful** for pure logic; routing matrix covers plan scenarios (hackintosh / macbookpro / tiny11)
- State management test coverage: **Partial** — Coordinator election + key relay lifecycle untested; no test for cursor-channel vs fleet-channel conflict
- UI component test coverage: **Partial** — `formatFleetGraph` only; no widget/poll integration test (acceptable for P5)

---

### Positive Observations

- Mesh v1/v2 gating is consistent across Coordinator, ServerApp, and Settings default — safe incremental rollout.
- Server-side fleet topology injection (`peekFleetNeighbor`, multi-hop walk, queued switch on pre-connect) directly addresses the "first edge cross" and offline-neighbor problems from the plan.
- `RelayKeyEvent` neutral payload cleanly decouples mesh protocol from Server/PrimaryClient inject.
- `adoptClient` + `resyncEnterIfActiveClient` fixes a real reconnect cursor desync.
- AutoModeRunner epoch-scoped screen enter/leave handlers prevent stale `ElectionState` cursor flags across restarts.

---

### Verdict

**Needs work** — fix the `CoordinationKeyForward` handler lifetime issue and mesh v2 cursor authority conflict before merge. Address naming and silent key-drop edge cases before enabling mesh v2 in production soak.
