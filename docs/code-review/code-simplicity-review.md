# Code Simplicity Review — Consolidation Sprint

**Date:** 2026-06-30  
**Reviewer:** @code-simplicity-review-agent  
**Scope:** VirtualHostTracker, HidConsumer, Server.cpp refactor, ServerProxy, ServerConfigDialog, Settings.h, related unit tests  
**Plan reference:** `docs/plan/2026-06-30-refactor-fork-consolidation-upstream-readiness-plan.md` (Phase 3)

---

## Executive summary

The sprint successfully extracts focus-follow connect/disconnect into `VirtualHostTracker` and introduces a client-side `HidConsumer` seam. Both are directionally correct and well-sized at the type level. However, **simplicity gains are only partial**: `Server.cpp` still carries parallel Mouser-bridge and HID-passthrough handlers with repeated lambdas, the `HidConsumer` abstraction adds indirection without yet delivering device-agnostic delivery, and settings/GUI wiring introduces a second configuration axis (`HidConsumer` string) alongside existing booleans. Tests cover encoding helpers but mislabel behavior and omit key tracker/proxy paths.

**Verdict:** Ship-ready for fork use; simplify before upstream PR stack.

| Severity | Count |
|----------|------:|
| Critical | 2 |
| Important | 7 |
| Suggestions | 6 |

---

## Critical

### C1 — Misnamed / misleading unit tests

**Files:** `src/unittests/client/HidConsumerTests.cpp`, `src/unittests/server/VirtualHostTrackerTests.cpp`

1. `HidConsumerTests::emptyBytesProduceNoDelivery` only calls `encodeHidReportAsMouserLine(1, {})` and asserts JSON output. It never constructs `MouserHidConsumer` or verifies that empty reports are skipped in `deliverRawReport()`. The name implies integration behavior that is untested.

2. `VirtualHostTrackerTests::detachClearsHost` calls `clearHostIf()` instead of `detach()`. The slot name and plan acceptance criterion (“focus A→B→primary disconnect/connect sequence”) imply `detach()` sends a disconnect line and clears host — that path is untested.

**Impact:** False confidence against Phase 3 acceptance (“`ServerProxy::hidReport` has test coverage for raw delivery path”).

**Recommendation:** Rename tests to match what they assert; add focused tests for `MouserHidConsumer::deliverRawReport` (null client, empty bytes, delivers when valid) and `VirtualHostTracker::detach` (sends custom disconnect line, clears host).

---

### C2 — Settings `HidConsumer` key is written selectively, never cleared

**Files:** `src/lib/gui/dialogs/ServerConfigDialog.cpp` (accept), `src/lib/client/HidConsumer.cpp` (`hidConsumerModeFromSettings`)

`accept()` sets `client/hidConsumer = "mouser"` only when HID passthrough is enabled. It never writes `"none"` when sharing is disabled, and gesture-only mode relies on `MouserEnabled` without touching `HidConsumer`.

`hidConsumerModeFromSettings()` resolves mode from **two** sources (string + bool), with comment “Default follows MouserEnabled.” Gesture-only and HID-passthrough paths therefore diverge in persisted shape even when runtime behavior matches.

**Impact:** Hand-edited or migrated configs can have `hidConsumer=mouser` with `mouserEnabled=false`; current logic returns `None` (safe today) but the dual axis is a footgun for future modes (`native`) and for upstream docs.

**Recommendation:** Pick one source of truth: either derive client mode solely from `MouserEnabled` until a second consumer exists, or always write `HidConsumer` in `accept()` for all three states (`none`, `mouser`).

---

## Important

### I1 — `sendMouser` lambda duplicated six times in `Server.cpp`

**File:** `src/lib/server/Server.cpp` (~558, 568, 607, 645, 657, 680)

The VirtualHostTracker extraction removed duplicated *logic* but not duplicated *wiring*. The identical lambda:

```cpp
const auto sendMouser = [](BaseClientProxy *client, const std::string &payload) {
  client->sendMouserData(payload);
};
```

appears in every call site. This was the main readability target of Phase 3.1 and still obscures the handlers.

**Recommendation:** One private `Server::sendMouserDataToClient(BaseClientProxy *, const std::string &)` or a file-local helper; pass `std::bind` / member pointer adapter to `VirtualHostTracker`.

---

### I2 — Parallel event handlers remain structurally duplicated

**File:** `src/lib/server/Server.cpp` — `handleMouserBridgeLine`, `handleHidPassthroughEvent`, `updateMouserVirtualHost`, `updateHidVirtualHost`

Attach/detach/relay patterns mirror each other:

| Step | Mouser bridge | HID passthrough |
|------|---------------|-----------------|
| Cache connect line | `setConnectLine(line)` | `setConnectLine(payload)` |
| Connect on remote focus | `onFocusChange(..., line)` | `onFocusChange(..., hidConnectLineForClient())` |
| Detach | `clearConnectLine` + `detach` | same + decode cache reset |
| Relay frames | `relaysTo` → `sendMouserData` | `relaysTo` → `sendHidFrame` |

VirtualHostTracker handles focus-follow; **handler shape** is still copy-pasted. HID-specific decode/seize deferral belongs in HID path, but connect/disconnect relay scaffolding could share a small helper.

**Recommendation:** Extract `handleVirtualHostAttach(VirtualHostTracker &, connectLine, sendFn)` / `handleVirtualHostDetach(...)` or template the send callback type to reduce branching duplication.

---

### I3 — `HidConsumer` virtual interface with a single implementation (YAGNI)

**Files:** `src/lib/client/HidConsumer.h`, `src/lib/client/ServerProxy.h`

Plan 3.3 envisions future `native` consumer. Current code has:

- `enum class HidConsumerMode { None, Mouser }` (two values)
- Abstract `HidConsumer` + concrete `MouserHidConsumer` only
- `std::unique_ptr<MouserHidConsumer>` in `ServerProxy`

The vtable, mode enum, and `hidConsumerOrNull()` indirection add ceremony before a second consumer exists. Plan originally suggested `MouserClient` implement the interface; instead an adapter wraps `MouserClient::deliver()`.

**Recommendation:** Until `native` is scheduled, a bool/`MouserEnabled` gate + direct `MouserClient` call in `hidReport()` is simpler. Re-introduce interface when second backend lands.

---

### I4 — Phase 3.3 goal only half met: raw bytes still re-encoded to Mouser JSON

**Files:** `src/lib/client/HidConsumer.cpp`, `src/lib/client/ServerProxy.cpp`

`ServerProxy::hidReport()` now routes through `HidConsumer`, but `MouserHidConsumer::deliverRawReport` immediately calls `encodeHidReportAsMouserLine` and `MouserClient::deliver`. The wire path is still Mouser JSON — the abstraction moved encoding but did not decouple it.

Dual ingress remains: `mouserData()` → direct deliver; `hidReport()` → encode → deliver.

**Recommendation:** Document as intentional migration step in code comment; track in FORK_ROADMAP. For simplicity, consider collapsing to one path (always HIDR + consumer) when Mouser bridge tier is decode-only.

---

### I5 — Duplicate hex encoding utilities (client vs server)

**Files:** `src/lib/client/HidConsumer.cpp` (`encodeHidReportAsMouserLine`), `src/lib/server/HidPassthrough.cpp` (`hidBytesToHex`)

Both implement lowercase nibble loops with `"0123456789abcdef"`. Server tests cover `hidBytesToHex`; client tests cover parallel logic in `encodeHidReportAsMouserLine`.

**Recommendation:** Shared helper in `deskflow` common (e.g. `ProtocolUtil` or tiny `BytesHex.h`) used by both libs; one test target.

---

### I6 — Phase 3 acceptance metric not met: virtual-host section still large

**Plan target:** “`Server.cpp` virtual-host logic ≤ 50 lines (delegated to tracker)”

Tracker delegation exists, but Mouser + HID bridge handlers, decode deferral, and focus updates span ~235 lines (516–753). LOC reduction from VirtualHostTracker is real but below the stated bar.

**Recommendation:** Treat ≤50 as aspirational or remeasure after I1/I2; do not claim Phase 3.1 complete in upstream PR description without noting remaining handler bulk.

---

### I7 — `ServerConfigDialog` mutual-exclusion UX adds state without fully hiding legacy toggle (Option A partial)

**Files:** `src/lib/gui/dialogs/ServerConfigDialog.cpp`, `Server.cpp` `initMouserBridge`

Runtime implements Option A partially: `initMouserBridge()` starts bridge when `HidPassthroughEnabled` even if `MouserBridgeEnabled` is false (decode sync). GUI hides gesture sharing when HID is on but still exposes separate persisted keys and couples `MouserEnabled` to `hidPassthrough || gestureShare`.

**Recommendation:** Align naming: persist an explicit `decodeSyncOnly` internal flag or document in UI that HID mode auto-starts bridge listener. Reduces cognitive load vs two group boxes + three settings keys.

---

## Suggestions

### S1 — Collapse `HidConsumerMode` to bool until third mode exists

`HidConsumerMode` + string setting + bool is three representations of two states.

---

### S2 — Reduce `VirtualHostTracker` accessor surface

`setConnectLine`, `clearConnectLine`, `hasConnectLine`, `connectLine` could become `setConnectLine(std::optional<std::string>)` or a single mutable cache API. Current API is test-friendly but verbose.

---

### S3 — Deduplicate default device pattern `046D:*`

**File:** `ServerConfigDialog.cpp` — repeated in `accept()`, `loadFromConfig()`, `isSharingModified()`.

Extract `constexpr` or `Settings` default constant.

---

### S4 — `VirtualHostTracker` uses `std::function` for send callback

Fine for unit tests. Production could use a function pointer or templated callback to avoid heap allocation on focus switches (minor).

---

### S5 — `ServerProxy` lazy-init split: `mouserClientOrNull` vs `hidConsumerOrNull`

Two lazy paths with nested null checks. When consumer is always Mouser-backed, single factory returning `(MouserClient*, HidConsumer*)` pair would shorten `hidReport()`.

---

### S6 — `VirtualHostTracker::relaysTo` naming

“Relays” is accurate for Mouser bridge; HID path relays raw frames via `sendHidFrame`. Consider `hostsDevice(active)` for domain-neutral naming in shared docs.

---

## What went well

| Area | Assessment |
|------|------------|
| **VirtualHostTracker** | Right-sized class (~80 LOC), clear responsibility, good focus-change semantics including `connectPayload` override for decode merge |
| **Extraction boundary** | Two tracker instances (`m_mouserVirtualHostTracker`, `m_hidVirtualHostTracker`) correctly model two independent virtual devices |
| **ServerProxy lazy init** | Mouser client created on first use; avoids cost when disabled |
| **Settings.h comments** | Fork keys documented inline; aids upstream reviewers |
| **HidPassthroughTests** | Solid coverage for decode merge helpers exercised by HID path |
| **ServerConfigDialog** | Mutual exclusion between HID and legacy gesture toggle is clear UX |

---

## Recommended simplification sequence (fork, pre-upstream)

1. Fix C1 tests (rename + add missing cases) — low risk, unblocks honest Phase 3 sign-off  
2. Resolve C2 settings single source of truth  
3. I1 — dedupe `sendMouser` lambda  
4. I5 — shared hex helper  
5. Defer I3/I4 until `native` consumer is on roadmap; document current JSON adapter as temporary  
6. I2 — optional handler helper if upstream PR needs further shrink  

---

## File inventory reviewed

| Path | Role |
|------|------|
| `src/lib/server/VirtualHostTracker.{h,cpp}` | Focus-follow virtual host |
| `src/lib/server/Server.{h,cpp}` | Integration, handlers |
| `src/lib/client/HidConsumer.{h,cpp}` | Client consumer seam |
| `src/lib/client/ServerProxy.{h,cpp}` | Protocol + lazy consumer |
| `src/lib/gui/dialogs/ServerConfigDialog.cpp` | GUI → settings |
| `src/lib/common/Settings.h` | Config keys |
| `src/unittests/server/VirtualHostTrackerTests.cpp` | Tracker tests |
| `src/unittests/client/HidConsumerTests.cpp` | Consumer tests |
| `src/unittests/server/HidPassthroughTests.cpp` | Related server helpers |

---

## Summary counts

| Severity | Count |
|----------|------:|
| **Critical** | 2 |
| **Important** | 7 |
| **Suggestions** | 6 |
