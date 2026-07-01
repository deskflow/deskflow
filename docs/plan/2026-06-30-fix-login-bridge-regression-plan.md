---
vgv_next:
  skill: build
  artifact: docs/plan/2026-06-30-fix-login-bridge-regression-plan.md
title: fix login-screen bridge regression
type: fix
date: 2026-06-30
---

## fix login-screen bridge regression — Standard

## Overview

Restore login-window KVM on macOS: controlling and unlocking a cluster Mac from the elected server while at the login screen. User reports this stopped working after the Jun 30 consolidation sprint. **Source analysis shows no direct edits** to `LoginBridgeManager` or `deskflow-vhid-bridge` in those commits — fix path is **diagnose-first** (Approach A from brainstorm), then apply the smallest corrective action per failure layer.

Brainstorm: [`docs/brainstorm/2026-06-30-login-bridge-regression-brainstorm-doc.md`](../brainstorm/2026-06-30-login-bridge-regression-brainstorm-doc.md)

## Problem Statement / Motivation

- **Symptom:** Cannot control/unlock login Mac from another machine at the login window.
- **User state:** Has not checked `/var/log/deskflow-vhid-bridge.log` yet.
- **Impact:** Core fork differentiator (Karabiner login bridge) appears broken after fleet update to consolidation `master`.
- **Likely class:** Install/ops (reinstall, agent plist, logout, Karabiner daemon, server not listening) — not a missing feature.

## Proposed Solution

### Phase 0 — Diagnose (no code)

Run on **login Mac** (machine to unlock):

| Step | Command / action | Pass criteria |
|------|------------------|---------------|
| 0.1 | `ls -la /Applications/Deskflow.app/Contents/MacOS/deskflow-vhid-bridge` | Binary exists, executable |
| 0.2 | `cat /Library/LaunchAgents/org.deskflow.vhid-bridge.plist` | Plist exists; `ProgramArguments` has peer hosts, screen name, port, `--scale` |
| 0.3 | `pgrep -fl Karabiner-VirtualHIDDevice-Daemon` | Daemon running |
| 0.4 | Deskflow → Settings → **Login Window** | Status = **Active** (or note exact error string) |
| 0.5 | Logout → login screen → `sudo tail -100 /var/log/deskflow-vhid-bridge.log` | Capture full output |

Run on **server Mac** (controlling machine):

| Step | Command / action | Pass criteria |
|------|------------------|---------------|
| 0.6 | Press **Start** (Accessibility granted) | Core runs |
| 0.7 | `pgrep -fl deskflow-core` | Process present while testing |
| 0.8 | Verify `coordination/peers` in `~/Library/Deskflow/Deskflow.conf` | Login Mac listed with reachable LAN IP |
| 0.9 | Auto mode: confirm elected server is the machine you expect | Server role at login time |

Record results in a short comment (issue or `docs/dev/login-bridge-triage-YYYY-MM-DD.md`).

### Phase 1 — Fix by failure layer (decision tree)

```
Log / status finding                    → Action
─────────────────────────────────────────────────────────────
Bridge binary missing                   → Reinstall: scripts/install-macos.sh
Karabiner driver not installed          → Install VirtualHIDDevice; approve extension
Daemon not running                      → Start daemon; reboot if needed
Agent plist missing / stale peers       → Re-enable Login Window tab; admin prompt; LOGOUT
"virtual HID not ready" in log          → Fix Karabiner stack (above)
Connection / handshake errors           → Fix server Mac Start + peers + port
"handshake complete" but no input       → Check screen name match, scale, display fallback in log
```

**Reinstall note:** Commit `fbd080bb7` changed nested-binary codesign order. Fleet machines updated before that commit may have an unsigned or missing `deskflow-vhid-bridge` in the bundle — reinstall is the fix.

**Logout note:** LoginWindow LaunchAgents do not load until logout/restart — re-enabling after reinstall without logout looks like a regression.

**Server note:** Accessibility gate on server Mac blocks `startCore()` until TCC granted — bridge cannot connect if no server listens.

### Phase 2 — Code hardening (only if Phase 0–1 insufficient)

Defer unless diagnostics show config/persistence/UX gaps:

| Item | File(s) | Change |
|------|---------|--------|
| P2.1 | `Settings.h`, `SettingsTests.cpp` | Add `coordination/loginBridgeEnabled` + `loginBridgeScale` to `m_validKeys`; unit test |
| P2.2 | `SettingsDialog.cpp` | Warn when peers/port/scale changed but agent plist may be stale; offer re-apply |
| P2.3 | `LoginBridgeManager.cpp` | Surface last N log lines in Settings tab (read `/var/log/deskflow-vhid-bridge.log`) |
| P2.4 | `INSTALL.md` | Login-bridge post-install checklist (enable → logout → test) |

**Out of scope:** Rewriting `deskflow-vhid-bridge`, Tier 2 HID passthrough, desktop Mouser virtual-host refactor.

## Technical Considerations

### Architecture (unchanged)

- Login bridge: `deskflow-vhid-bridge` → Karabiner virtual HID → loginwindow session
- Desktop KVM: `deskflow-core` → CGEvent / hooks (requires Accessibility on server)
- No runtime conflict between HID passthrough seize (logged-in) and login bridge (LoginWindow session)

### What consolidation changed (indirect only)

| Commit | Login impact |
|--------|--------------|
| `fbd080bb7` | Codesign order — reinstall may be required |
| `e4c7052bb` | Auto-mode stability — helps server availability |
| Accessibility gate | Blocks server Start — bridge needs listening peer |
| `f64343c45`, `04b322638` | Desktop Mouser/HID only |

### Latent bug

`loginBridge*` keys not in `m_validKeys` — safe today (`coordination` ∉ `m_validGroup`); fix in Phase 2.1 preventively.

## Acceptance Criteria

- [ ] `/var/log/deskflow-vhid-bridge.log` captured for failed attempt and root cause identified
- [ ] Login Mac Settings → Login Window shows **Active** with Karabiner daemon running
- [ ] From server Mac, mouse and keyboard work at login Mac's login window
- [ ] Survives logout → login screen test without manual re-enable (after one-time setup)
- [ ] Documented fix path in triage notes (which layer failed: binary / Karabiner / plist / server)

## Success Metrics

- Login unlock works on user's cluster after following Phase 0–1
- Zero false "regression" reports from missing logout or stale install
- If Phase 2 shipped: `SettingsTests` covers login-bridge key persistence

## Dependencies & Risks

| Risk | Mitigation |
|------|------------|
| Karabiner DriverKit not approved | GUI link to releases; status line |
| Peers use hostname not LAN IP at login | Use `name=192.168.x.x` in `coordination/peers` |
| Server not elected at login | Ensure auto-mode core runs on server Mac before logout test |
| User tests before checking log | Phase 0 mandatory before any code |

## References & Research

- Brainstorm: `docs/brainstorm/2026-06-30-login-bridge-regression-brainstorm-doc.md`
- `src/lib/gui/LoginBridgeManager.cpp` — agent install, Karabiner checks
- `src/apps/deskflow-vhid-bridge/deskflow-vhid-bridge.cpp` — client handshake
- `src/lib/gui/dialogs/SettingsDialog.cpp` — Login Window tab (macOS)
- `docs/coordination/behavior-spec.md` — login-screen behavior
- `INSTALL.md` — login bridge overview
- Log path: `/var/log/deskflow-vhid-bridge.log`

## Implementation Tasks

### Phase 0 — Diagnose

- [ ] **0.1** Run login Mac checklist (binary, plist, daemon, GUI status)
- [ ] **0.2** Run server Mac checklist (Start, core process, peers)
- [ ] **0.3** Logout test + capture bridge log
- [ ] **0.4** Map log line to decision-tree row; record root cause

### Phase 1 — Remediate (ops)

- [ ] **1.1** Apply fix for identified layer (reinstall / Karabiner / re-enable agent / server Start / peers)
- [ ] **1.2** Re-test login unlock end-to-end
- [ ] **1.3** If fixed with ops only, close as operator error / stale install — no code required

### Phase 2 — Hardening (conditional)

- [ ] **2.1** `Settings.h` + `SettingsTests.cpp` — loginBridge keys in `m_validKeys` (done)
- [ ] **2.2** `SettingsDialog.cpp` — stale plist warning / re-apply (done)
- [ ] **2.3** `LoginBridgeManager` — log tail in GUI (done)
- [ ] **2.4** `INSTALL.md` — post-install login bridge checklist (done)

## Test Plan

**Manual (required):**

1. Fresh install via `scripts/install-macos.sh` on login Mac
2. Install Karabiner VirtualHIDDevice + approve extension
3. Configure `coordination/peers` on both machines
4. Enable Login Window bridge → admin → **logout**
5. From server Mac, verify pointer + password entry at login screen
6. Inspect `/var/log/deskflow-vhid-bridge.log` for `handshake complete`

**Automated (Phase 2 only):**

- `SettingsTests::loginBridgeKeysSurviveCleanSettings` (new)
