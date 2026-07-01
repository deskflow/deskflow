---
vgv_next:
  skill: plan
  artifact: docs/brainstorm/2026-06-30-fleet-keyboard-follow-cursor-brainstorm-doc.md
date: 2026-06-30
topic: fleet-keyboard-follow-cursor
---

# Fleet keyboard follow-cursor (auto mode)

## What We're Building

In **auto-switch mode** with **3+ machines**, keyboard input typed on **any** peer should reach whichever machine/monitor currently holds the **fleet cursor** — without changing auto-switch leadership. If the cursor is on the same machine you typed on, keys behave normally (local OS). If the cursor is elsewhere, keystrokes are **silently forwarded** to that machine.

Today, only the **server epoch** captures physical keyboard input and routes it to `m_active` (the Deskflow screen under the cursor). Clients discard local keyboard for KVM purposes except when they **promote** via genuine mouse/touch input. This feature closes that gap for a multi-machine fleet.

## Why This Approach

Three directions were considered:

| Approach | Verdict |
|----------|---------|
| **Coordination mesh extension** | **Chosen** — server already knows active screen; extend 24851 JSON with cursor-host + key-forward messages; minimal change to promotion/election |
| Deskflow protocol reverse channel | Rejected — couples keyboard relay to 24800 client session lifecycle; harder during epoch flips |
| Centralized all-peer capture on server | Rejected — heavy refactor; fights current server/client epoch model |

The coordination layer already exists for election (`claim`, `promote`, `status`). Cursor location and key forwarding are **fleet metadata**, not Deskflow screen-layout data — they belong on the mesh, not in `ServerConfig`.

### Grounding in current code

- **Keyboard routing (server):** `Server::onKeyDown` → `m_active->keyDown(...)` unless keyboard broadcasting is enabled (`Server.cpp`).
- **Active screen:** `Server::switchScreen` sets `m_active` when the cursor crosses layout edges; server is authoritative today within one epoch.
- **Auto mode:** `AutoModeRunner` runs server/client epochs; `Coordinator` handles promotion; `notifyCursorHere` is **only** used to tighten promotion debounce when cursor is on a client — not for fleet-wide cursor broadcast.
- **Promotion input:** genuine hardware detection (CGEventTap PID 0 / Raw Input `hDevice`) — keyboard forwarding must **not** trigger `onLocalInput()` promotion.

## Key Decisions

- **Scenario:** Auto-switch fleet (3+ peers), not single-machine multi-monitor.
- **Routing rule:** Forward keys when cursor is on another machine; local OS receives keys when cursor is here.
- **Cursor authority:** Server tracks active screen/client and publishes cursor host to the fleet.
- **Promotion:** Keyboard never promotes — mouse/touch keeps current touch-to-promote semantics.
- **Transport:** Extend coordination mesh (port 24851) with new message types; server injects via existing `m_active` Deskflow path.
- **YAGNI:** No new GUI in v1; settings toggle + logging. No change to keyboard-broadcast hotkey behavior initially.

## Proposed wire protocol (sketch)

### `cursor` — server → all peers (on screen switch + heartbeat)

```json
{"t": "cursor", "host": "<screen-or-machine-name>", "seq": 42}
```

- Emitted when `Server::switchScreen` changes `m_active` and periodically (~1 s) while server.
- `host` maps to the Deskflow screen name for the machine under the cursor (existing canonical names from layout / `core/computerName`).

### `keyfwd` — any peer → server (when local keyboard used and cursor elsewhere)

```json
{"t": "keyfwd", "from": "<sender>", "phase": "down|up|repeat", "id": 65543, "mask": 8, "button": 0, "lang": ""}
```

- Server validates token, maps to `KeyID`/`KeyModifierMask`, calls existing `onKeyDown`/`onKeyUp`/`onKeyRepeat` targeting `m_active`.
- **Do not** call `ElectionState::onLocalInput()` for these events.

### Client-side listener

Each machine (server **and** client epochs) runs a lightweight **local keyboard tap** when auto mode is enabled:

1. Subscribe to `cursor` messages → cache `cursorHost`.
2. On local key event: if `cursorHost != selfName`, encode `keyfwd` to current server address (from coordination state); else pass through to OS (no capture).
3. On server epoch: server already captures keys locally — skip `keyfwd` for keys originating on server when cursor elsewhere is handled by existing `m_active` path (avoid double-send).

## Happy path

1. Hackintosh is server; cursor on MacBook screen (`m_active` = macbook client).
2. User types on Windows laptop keyboard.
3. Windows sees `cursor.host == macbook`, sends `keyfwd` to Hackintosh server.
4. Server injects into MacBook via existing client proxy — MacBook receives keystrokes.
5. User moves mouse on Hackintosh — promotion unchanged; cursor moves; new `cursor` broadcast updates fleet.

## Edge cases & open questions

| Topic | Notes for planning |
|-------|-------------------|
| **Modifier sync** | Cmd on Mac → Ctrl on Windows when forwarding? Reuse existing `ServerProxy` key translation or sync raw modifiers? |
| **Key repeat** | Cross-network repeat timing; server may need to synthesize repeat if only `keyfwd down` arrives |
| **Secure Input (macOS)** | Password fields block injection on destination; forward anyway and let destination drop, or suppress `keyfwd` when secure input active on target? |
| **Login screen** | MacBook at login window uses `deskflow-vhid-bridge`; does `keyfwd` → server → bridge path work, or need bridge-local listener? |
| **Epoch flip mid-keystroke** | Server changes during held key — release all keys on role change (existing `leaveSecondary` pattern) |
| **Auth** | Reuse `coordination/token`; reject unsigned `keyfwd` |
| **Latency** | Accept LAN/Tailscale RTT; no buffering v1 |
| **Multi-monitor granularity** | v1: machine-level `host` (screen name = computer name in auto layout). Per-monitor within one machine is already Deskflow `m_active` on server — no mesh change needed unless layout uses multiple screens per host |
| **Promotion tap isolation** | Keyboard tap for `keyfwd` must use a separate code path from genuine-input promotion tap, or filter key events out of promotion counter |

## Out of scope (v1)

- Typing promotes server (explicitly rejected)
- Replacing mouse/touch promotion
- Keyboard broadcast to multiple screens simultaneously
- GUI configuration beyond a settings flag (`coordination/keyboardFollowCursor=true`)

## Success criteria

- [ ] 3-machine auto cluster: type on any non-cursor machine → text appears on machine under cursor
- [ ] Type on machine that has cursor → local apps receive keys (no forwarding)
- [ ] Keyboard activity does not cause server promotion or epoch churn
- [ ] Mouse/touch promotion still works as today
- [ ] Role flip clears stuck keys (no ghost modifiers)
