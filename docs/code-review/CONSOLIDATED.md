# Consolidated Code Review — Consolidation Sprint (Phase 0–3)

**Scope:** Fork hygiene + VirtualHostTracker + HidConsumer refactor + review fixes  
**Date:** 2026-06-30  
**Status:** All findings addressed

## Summary

| Severity | Original | Fixed |
|----------|----------|-------|
| **Critical** | 3 | 3 |
| **Important** | 10 | 10 |
| **Suggestions** | 12 | 12 |

---

## Critical — FIXED

| ID | Issue | Fix |
|----|-------|-----|
| C1 | HID settings stripped on load | `m_validKeys` + `SettingsTests::hidPassthroughKeysSurviveCleanSettings` |
| C2 | Misleading tests | Renamed/expanded; `detach()`, delivery guards, `connectPayload` override |
| C3 | No `hidReport` delivery tests | `deliverRawHidReport` / `deliverRawHidReportToMouser` unit tests |

---

## Important — FIXED

| ID | Issue | Fix |
|----|-------|-----|
| I1 | `forceLeaveClient` skips virtual-host teardown | Calls `updateMouserVirtualHost` + `updateHidVirtualHost` |
| I2 | `hidConsumer` never reset | Single source: `client/mouserEnabled`; removed dual-axis mode enum |
| I3 | Duplicate `sendMouser` lambdas | `sendMouserLine()` + `virtualHostOnFocusChange()` helpers |
| I4 | `MouserBridgeEnabled` lies when HID-only | OR with HID in dialog; `server/mouserBridgeActive` runtime key |
| I5 | Dual `VirtualHostTracker` conflict | `detachOtherVirtualHosts()` — only one host at a time |
| I6 | ServerConfigDialog cross-writes client settings | Documented fork behavior in `accept()` comment |
| I7 | Default `046D:*` over-broad | No default on save; placeholder only; validation on enable |
| I8 | `HidConsumer` YAGNI | Collapsed to `mouserHidDeliveryEnabled()` + callback delivery |
| I9 | Mouser/HID handler duplication | `virtualHostAttachIfRemote` / `virtualHostDetach` helpers |
| I10 | Sync script duplicate coordination keys | awk guard in `sync-debug-settings-macos.sh` |

---

## Suggestions — FIXED

- `VirtualHostTracker` thread-affinity comment
- `kMaxHidReportPayloadBytes` payload cap
- Collapsed `HidConsumerMode` → `bool mouserHidDeliveryEnabled()`
- Deduped `046D:*` → `SharingConstants::kHidDevicesPlaceholder` (placeholder only)
- Merged lazy-init → single `mouserDeliveryOrNull()` in `ServerProxy`
- Renamed `relaysTo` → `hostsActiveClient`
- `server/mouserBridgeActive` status key (core writes on bridge start/stop)
- `deskflow-core` `App` leak → `std::unique_ptr<App>`
- `VirtualHostTracker::detach()` + `connectPayload` override tests
- `mouserHidDeliveryEnabled` settings test
- `docs/hid-passthrough.md` decode handoff + client sink updated
- Shared `deskflow::bytesToLowerHex()` in `common/BytesHex.h`
- `VirtualHostTracker` templated send callback (no `std::function` heap alloc on focus path)
- Slim tracker API: `setConnectLine({})` clears cache; removed `clearConnectLine`

---

## Verdict

**All review findings resolved.** Ready to commit.
