---
vgv_next:
  skill: plan
  artifact: docs/brainstorm/2026-06-30-fix-fleet-keyboard-follow-cursor-relay-brainstorm-doc.md
date: 2026-06-30
topic: fix-fleet-keyboard-follow-cursor-relay
---

# Fix fleet keyboard follow-cursor relay

## What We're Building

Ship a **code-only fix** for fleet keyboard follow-cursor in auto mode (3+ peers). The feature landed in `7d4c1b50` but typing on a non-cursor machine does not reach the machine under the fleet cursor.

Root cause from log + code review: the client keyboard relay gates forwarding on a **non-empty `fleetCursorHost`** received via coordination mesh `cursor` messages. In practice that field is often **empty** on clients — mesh delivery is unreliable when peer lists use bare names (`macbookpro`) instead of Tailscale|LAN addresses, and server epochs flip frequently. When `fleetCursorHost` is empty, `OSXKeyboardRelayMonitor` passes every key to the local OS and never sends `keyfwd`.

The fix: use **`ElectionState::cursorHere()`** — already wired from `CoordinationScreenEntered` / `CoordinationScreenLeft` on client epochs — as the relay trigger. Forward keys when **client epoch AND cursor is not here**; do not require mesh cursor messages for the v1 decision.

## Why This Approach

Three directions were considered:

| Approach | Verdict |
|----------|---------|
| **`cursorHere()` for relay decision** | **Chosen** — data already exists and is accurate on client epochs; server still routes via `m_active`; no mesh dependency for the critical path |
| Fix cursor mesh delivery only | Rejected alone — keeps fragile dependency on peer address formatting and heartbeat rebroadcast; doesn't help when client never receives first `cursor` |
| Hybrid (cursorHere + mesh + INFO logs) | Deferred — user chose code-only, minimal scope; mesh cursor can remain for future fleet-wide visibility but is not the relay gate |

### Code gaps identified (what we missed in v1)

1. **Relay gate (`OSXKeyboardRelayMonitor.mm`, `MSWindowsKeyboardRelayMonitor.cpp`)**  
   `if (cursorHost.empty() || hostsEqual(cursorHost, selfHost)) return event;` — empty host silently disables forwarding.

2. **Unused signal already in production**  
   `AutoModeRunner` wires `CoordinationScreenEntered/Left` → `notifyCursorHere()`. This tracks whether the shared cursor is on the local screen during client epoch — exactly the condition needed for relay, but relay reads `fleetCursorHost()` instead.

3. **Mesh cursor is redundant for relay on client**  
   Server already knows `m_active` and injects forwarded keys there. Client only needs to know “cursor not here → send keyfwd to server.” Target screen is server’s problem.

4. **Server epoch path unchanged**  
   When local machine is server and cursor is on a remote screen, existing `Server::onKeyDown` → `m_active` already handles keyboard. Relay stays disabled on server epoch (per original plan). No change needed unless that path also fails (separate bug).

5. **Observability gap (out of scope per user)**  
   Relay start/stop, keyfwd send/receive, and cursor mesh are DEBUG-only or unlogged. SSH logging on macbook was deferred; optional INFO lines can be added in plan if cheap.

6. **Config asymmetry (deployment note, not code)**  
   Hackintosh `peers="macbookpro, hackintosh, tiny11"` vs macbook full `name=tailscale|lan` list affects mesh `cursor` delivery but is **not blocking** once relay uses `cursorHere()`.

## Key Decisions

- **Scope:** Code fix only — no SSH logging setup, no peer-config migration in this effort (can be done manually anytime).
- **Relay trigger:** Client epoch + `!cursorHere()` → intercept keys, `sendKeyForward` to server; swallow event (`return nullptr` / equivalent on Windows).
- **Relay pass-through:** Client epoch + `cursorHere()` → return event unmodified (local OS).
- **Server epoch:** Relay monitor stopped (unchanged); server uses existing primary capture + `m_active`.
- **Promotion:** Keyboard relay tap remains separate from `ILocalInputMonitor` (mouse-only promotion). No change.
- **Mesh `cursor` messages:** Keep broadcast for future use / heartbeat sync; **do not** require them for relay. Optionally still update `m_fleetCursorHost` when received.
- **YAGNI:** Do not add GUI toggle changes, login-bridge keyboard, or peer-list auto-normalization in this fix.

## Happy path (after fix)

1. MacBook is server; cursor on Hackintosh (`m_active` = hackintosh client).
2. User types on MacBook keyboard while MacBook is **client epoch** (following Hackintosh server) — wait, if MacBook is server, relay is off on MacBook.
3. Correct scenario: Hackintosh is **client**, cursor on MacBook (screen left on Hackintosh → `cursorHere=false`).
4. User types on Hackintosh → relay forwards `keyfwd` to MacBook server → server `relayForwardedKey` → `m_active` (MacBook local screen) → keystrokes appear on MacBook.

Alternate: Hackintosh is **server**, cursor on MacBook. User types on Hackintosh → `Server::onKeyDown` → `m_active` (MacBook client) — existing path, no relay.

## Edge cases

| Case | Behavior |
|------|----------|
| Cursor on self, client epoch | Keys local (pass-through) |
| Cursor elsewhere, client epoch | keyfwd to server |
| Server epoch, cursor on remote | Existing server primary path |
| Server epoch, cursor on self | Local keys on server |
| Epoch flip mid-key | Existing KeyState / leaveSecondary patterns; verify no stuck keys |
| Input Monitoring denied (macOS) | WARN at relay start; keys stay local (unchanged) |
| keyboardFollowCursor=false | Relay stopped (unchanged) |

## Open Questions

- Should `Coordinator::fleetCursorHost()` query be replaced entirely, or expose a new `shouldForwardKeyboard()` that combines role + cursorHere?
- Do we need a unit test on relay decision logic (mock cursorHere callback) separate from platform taps?
- After fix, is server-epoch typing to remote screen also broken in the field? If yes, that’s a separate investigation (not mesh/relay).
