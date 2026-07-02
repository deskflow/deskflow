# Architecture Review — Fleet mesh v2 refactor (P2–P5)

**Scope:** Branch `refactor/fleet-state-hub` vs `master`, including uncommitted P2–P5 work  
**Date:** 2026-07-02  
**Reviewer:** architecture-review-agent  
**Plan reference:** `docs/plan/2026-07-02-refactor-fleet-state-hub-mesh-v2-plan.md`

---

## Executive summary

The mesh v2 refactor largely follows the planned hub-and-spoke model: `Coordinator` owns transport and `FleetState`, `AutoModeRunner` is the glue epoch loop, `Server` consumes topology through a server-local `TopologyLink` adapter, and keyboard relay crosses layers via `RelayKeyEvent` + `CoordinationKeyForward` events. `meshVersion` gating in `Coordinator` is intentional and consistently applied.

**Two layer violations block a clean merge:** `platform/` and `gui/LoginBridgeManager` link the full `coordination` static library for localhost polling helpers that the plan explicitly intended to keep link-free. Source-level `server/` headers are clean (no `CoordinationProtocol.h`), but CMake propagates a full `coordination` dependency through `server` → `app`.

**Verdict:** Fix 2 critical link-layer violations and 1 important CMake dependency before merging P5.

---

## Target layer model (from plan)

| Layer | Responsibility | May depend on |
|-------|----------------|---------------|
| `coordination/` | Mesh transport, election, FleetState merge, protocol codec | `base`, `common`, `deskflow/KeyTypes` |
| `server/` | KVM handoff, key inject, topology adjacency | `RelayKeyEvent`, server-local types, events |
| `deskflow/` (`app`) | ServerApp/ClientApp glue, epoch lifecycle | `server`, coordination *types/events* via callbacks |
| `gui/` | Status display, settings | Localhost JSON poll (`127.0.0.1:24851`), not mesh internals |
| `platform/` | OS screens, hooks, watchdog | `base`, `client`, OS APIs — not mesh |
| `apps/deskflow-core/` | Orchestration (`AutoModeRunner`) | All of the above |

**Explicit plan rule:** `server/` must not `#include "coordination/CoordinationProtocol.h"`. Use `RelayKeyEvent` + events. GUI and login/HID consumers read fleet via localhost status, not by linking `coordination`.

---

## Layer Separation

**Violations found: 2 (critical)**

| File | Violation |
|------|-----------|
| `src/lib/platform/MSWindowsWatchdog.cpp:16-17` | **platform** imports `coordination/CoordinationLocalStatus.h` and `coordination/KeyboardRouter.h` |
| `src/lib/gui/LoginBridgeManager.cpp:10` | **gui** imports `coordination/CoordinationLocalStatus.h` (macOS login-bridge plist generation) |

**Build-manifest violations (important, see Dependency Direction):**

| File | Violation |
|------|-----------|
| `src/lib/platform/CMakeLists.txt:179` | `platform` links `coordination` on WIN32 |
| `src/lib/gui/CMakeLists.txt:145` | `gui` links `coordination` on APPLE |
| `src/lib/server/CMakeLists.txt:48` | `server` PUBLIC-links `coordination` (transitive to `app`) |

**Clean at source level:**

- `src/lib/server/Server.h:14` — only `coordination/RelayKeyEvent.h` (approved neutral type)
- `src/lib/server/PrimaryClient.h:10` — only `RelayKeyEvent.h`
- `src/lib/server/Server.cpp`, `PrimaryClient.cpp` — no protocol or Coordinator imports
- `src/lib/gui/CoordinationStatus.cpp` — polls `{"t":"status"}` over `QTcpSocket`; zero coordination includes
- `src/lib/gui/MainWindow.cpp` — consumes `CoordinationStatus` signals only
- `src/apps/deskflow-vhid-bridge/deskflow-vhid-bridge.cpp` — raw TCP status poll (`refresh_hosts_from_coord_snapshot`); no coordination link
- `src/apps/deskflow-core/AutoModeRunner.cpp` — appropriate orchestration layer; owns `Coordinator`
- `src/lib/coordination/` — no imports from `server/`, `gui/`, or `platform/`

**Acceptable glue-layer coupling (deskflow/app):**

- `src/lib/deskflow/ServerApp.h:13-14` — `CoordinationEvents.h`, `FleetState.h`
- `src/lib/deskflow/ClientApp.h:12` — `RelayKeyEvent.h`
- `src/lib/deskflow/ServerApp.cpp:691-758` — converts `FleetLink` → `server::TopologyLink` before touching `Server`

This is expected: `ServerApp` is the adapter between coordination snapshots and the KVM server. The boundary at `Server::setFleetTopologyLinks(std::vector<TopologyLink>)` is well drawn.

---

## State Management Assessment

Mesh v2 does not use Flutter/BLoC; state is mutex-guarded snapshots + event bus. Assessment against project patterns:

| Unit | Verdict | Notes |
|------|---------|-------|
| `FleetState` / `FleetFragment` | **Correct** | Immutable-friendly POD structs in `FleetState.h`; no Qt, no KeyTypes |
| `FleetStateMerge` | **Correct** | Pure `applyServerFragment()` — tested like `ElectionState` |
| `Coordinator::fleetSnapshot()` | **Correct** | Mutex-guarded copy (`Coordinator.h:117-118`); single writer |
| `KeyboardRouter` | **Correct** | Pure `routeKeyboard()` / `cursorHostIsLocal()`; no I/O |
| `Coordinator` meshVersion gates | **Correct** | v1/v2 code paths separated at `Coordinator.cpp` (hello, fleet, key, keyfwd) |
| `ServerApp` fleet handlers | **Correct** | Subscribes to `CoordinationFleetStateChanged` / `CoordinationTopologyReady`; pushes `TopologyLink` into `Server` |
| `CoordinationStatus` (GUI) | **Correct** | Timer poll → parse JSON → emit signals; no mesh ownership |
| `AutoModeRunner` | **Correct** | Sets callbacks on `ServerApp`/`ClientApp`; does not leak Coordinator into `server/` |

**Minor note:** `ServerApp::publishFleetTopologyFromConfig()` builds `coordination::FleetLink` vectors directly (`ServerApp.cpp:92-113`). This is glue-layer leakage of coordination DTOs into `deskflow/`, but it stops at the publish callback and does not reach `server/`. Acceptable for P2–P5; could be neutralized later.

---

## Dependency Direction

**Direction violations: 3**

### 1. platform → coordination (critical)

```
platform (MSWindowsWatchdog)
  → coordination (CoordinationLocalStatus, KeyboardRouter)
    → CoordinationMesh, CoordinationProtocol, Coordinator internals
```

`MSWindowsWatchdog::wantsElevatedCore()` (`MSWindowsWatchdog.cpp:397-412`) only needs:
- A localhost status poll (cursor host name)
- `cursorHostIsLocal(self, cursorHost)` (3-line pure function)

Linking the full `coordination` static library pulls mesh transport, election, and input monitors into the platform layer — the lowest OS-integration tier. This inverts the intended graph and couples UAC elevation policy to mesh internals.

### 2. gui → coordination (critical, macOS only)

```
gui (LoginBridgeManager)
  → coordination (CoordinationLocalStatus::pollLocalFleetStatus)
```

Plan P5 / technical review I4: *"deskflow-vhid-bridge can't link coordination lib — localhost snapshot endpoint."* The bridge correctly uses inline TCP poll; `LoginBridgeManager` does not. `CoordinationStatus` already demonstrates the correct GUI pattern (no coordination link). `LoginBridgeManager` should match it.

### 3. server PUBLIC → coordination (important)

```
server --[PUBLIC]--> coordination --[PUBLIC]--> Qt6::Network, ...
app (deskflow) --[PRIVATE, UNIX]--> server
```

`server/CMakeLists.txt:48` propagates `coordination` to every `server` consumer. On UNIX, `app` links `server` (`deskflow/CMakeLists.txt:108`), so the entire Deskflow application library transitively depends on the mesh stack even though `Server.h` only includes `RelayKeyEvent.h`.

**Clean dependencies:**

- `coordination` → `common`, `base`, `deskflow/KeyTypes` (no `server/`, no `gui/`)
- `deskflow-core` → `coordination`, `server`, `app` (orchestrator; correct)
- `server` headers → `RelayKeyEvent` only (source-clean)
- Event bus: `CoordinationKeyForward`, `CoordinationFleetStateChanged`, `CoordinationTopologyReady` in `base/EventTypes.h` — consumers react without importing protocol

**No circular dependencies detected.**

---

## Package Structure

| Package | Status | Findings |
|---------|--------|----------|
| `coordination/` | **Complete** | New files (`FleetState.h`, `FleetStateMerge`, `KeyboardRouter`, `RelayKeyEvent`, `CoordinationLocalStatus`) added to `CMakeLists.txt`. Unit tests present for merge, protocol, router, local status, fleet publish. |
| `server/` | **Mostly complete** | `TopologyLink.h` is a proper server-local edge type. **Issue:** PUBLIC link to `coordination` should be narrowed. |
| `deskflow/` (`app`) | **Complete** | No CMake link to `coordination`; uses headers via include path. Glue callbacks in `ServerApp`/`ClientApp`. |
| `gui/` | **Split** | `CoordinationStatus` — correct thin client. `LoginBridgeManager` — links `coordination` on APPLE. |
| `platform/` | **Issue** | WIN32 links `coordination` for watchdog elevation heuristic only. |
| `apps/deskflow-core/` | **Complete** | `AutoModeRunner` is the documented glue; links `coordination` explicitly. |
| `apps/deskflow-vhid-bridge/` | **Complete** | Standalone; inline status poll; no Qt, no coordination lib. |

### CoordinationLocalStatus placement

`CoordinationLocalStatus` (`coordination/CoordinationLocalStatus.cpp`) is a localhost JSON client, not mesh transport. Placing it inside the `coordination` library forces poll-only consumers (`gui`, `platform`, potentially others) to link the entire mesh. The plan intended this as a **read-only localhost contract** shared by GUI, vhid-bridge, and watchdog.

**Recommendation:** Move to `common/` or a new `coordination-client/` header-only target (Qt TCP poll + JSON parse). `KeyboardRouter.h` (`cursorHostIsLocal` only) could move with it or be duplicated as a one-liner in `common/`.

---

## meshVersion flag (intentional)

Reviewed and confirmed intentional:

- `Settings::Coordination::MeshVersion` read in `AutoModeRunner.cpp:99,183-184`
- `Coordinator.cpp` gates hello, fleet publish/consume, key vs keyfwd, cursor-host relay, status fleet embedding
- Legacy v1 paths preserved when `meshVersion < 2`

No architectural concern with dual-protocol operation behind the flag; cutover in P6 is the planned hard break.

---

## Positive patterns (keep)

1. **Server topology adapter** — `FleetLink` (coordination) → `TopologyLink` (server) with `Direction` enum conversion in `ServerApp::applyFleetTopologyFromSnapshot()`.
2. **Neutral keyboard payload** — `RelayKeyEvent` decouples `Server`/`PrimaryClient` inject from mesh codec.
3. **Event-driven fleet updates** — `ServerApp` listens for `CoordinationFleetStateChanged` rather than polling Coordinator.
4. **GUI status bar** — `CoordinationStatus` polls localhost; `formatFleetGraph()` is presentation-only.
5. **Pure merge/router** — `FleetStateMerge` and `KeyboardRouter` are testable without transport mocks.
6. **vhid-bridge** — respects the no-link constraint with inline TCP status parsing.

---

## Recommended fixes (by priority)

### Critical

1. **Extract `CoordinationLocalStatus` out of `coordination` library** into `common/` (or thin client lib). Update `gui/LoginBridgeManager` and `platform/MSWindowsWatchdog` to depend on the extracted target, not `coordination`.
2. **Remove `target_link_libraries(gui coordination)`** (APPLE) and `target_link_libraries(platform coordination)` (WIN32) once extraction is done.

### Important

3. **Narrow `server` CMake dependency** — either:
   - Move `RelayKeyEvent.h` to `common/` or `server/` and drop `server` → `coordination` link entirely, or
   - Change to `target_link_libraries(server PUBLIC common)` + include path for header-only `RelayKeyEvent`, avoiding PUBLIC propagation of the full mesh lib.
4. **Deduplicate localhost poll logic** — `deskflow-vhid-bridge` hand-rolls JSON parsing (`refresh_hosts_from_coord_snapshot`); `CoordinationLocalStatus` uses `QJsonDocument`. Consider a shared spec test fixture so the two parsers stay aligned.

### Suggestions

5. **Neutralize `ServerApp` publish callback** — accept `std::vector<TopologyLink>` or a `deskflow`-local edge struct instead of `coordination::FleetLink` in the public `ServerApp` API.
6. **Document layer rules** in `docs/coordination/behavior-spec.md` § consumer contract (localhost poll vs link) so future P5 consumers don't repeat the LoginBridgeManager pattern.

---

## Verdict

**Fix 2 critical violations before merging P5.**

Source-level `server/` separation is good — no `CoordinationProtocol` leaks, `TopologyLink` adapter is clean, events carry keyboard relay. The remaining problems are **CMake link boundaries**: `platform/` and `gui/LoginBridgeManager` link the full mesh library for localhost polling that the plan designed as a link-free contract. `server` PUBLIC-linking `coordination` undermines the header-only `RelayKeyEvent` discipline.

---

## Files reviewed

**coordination/** — `Coordinator.{h,cpp}`, `CoordinationProtocol.{h,cpp}`, `CoordinationLocalStatus.{h,cpp}`, `CoordinationEvents.h`, `FleetState.h`, `FleetStateMerge.{h,cpp}`, `KeyboardRouter.{h,cpp}`, `RelayKeyEvent.h`, `CMakeLists.txt`

**server/** — `Server.{h,cpp}`, `PrimaryClient.{h,cpp}`, `TopologyLink.h`, `CMakeLists.txt`

**deskflow/** — `ServerApp.{h,cpp}`, `ClientApp.{h,cpp}`, `CMakeLists.txt`

**gui/** — `CoordinationStatus.{h,cpp}`, `LoginBridgeManager.cpp`, `MainWindow.cpp`, `CMakeLists.txt`

**platform/** — `MSWindowsWatchdog.{h,cpp}`, `CMakeLists.txt`

**apps/** — `deskflow-core/AutoModeRunner.{h,cpp}`, `deskflow-vhid-bridge/deskflow-vhid-bridge.cpp`, `deskflow-daemon/DaemonApp.cpp`
