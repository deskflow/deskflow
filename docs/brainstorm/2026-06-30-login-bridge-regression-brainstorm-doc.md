---
vgv_next:
  skill: plan
  artifact: docs/brainstorm/2026-06-30-login-bridge-regression-brainstorm-doc.md
date: 2026-06-30
topic: login-bridge-regression
---

# Login-screen bridge regression — brainstorm

## What We're Building

Restore **login-window KVM**: controlling and unlocking a Mac from another machine in the cluster while macOS shows the login screen, using the bundled `deskflow-vhid-bridge` and Karabiner DriverKit virtual HID.

User report: after the consolidation sprint (Jun 30), login-screen control **stopped working**. User has **not yet checked** `/var/log/deskflow-vhid-bridge.log`. Consolidation commits did **not** edit `LoginBridgeManager` or `deskflow-vhid-bridge` source — so the failure is likely **environmental, install, or upstream-of-bridge** (peers, server availability, Karabiner stack, stale LaunchAgent), not a direct code deletion.

Scope of this brainstorm: **login unlock only** (not desktop Mouser/gestures).

## Why This Approach

We split login-bridge regressions from desktop HID/Mouser regressions because they use different binaries, sessions, and dependencies. The Jun 30 refactor (`VirtualHostTracker`, `ServerConfigDialog` sharing move, `HidConsumer` collapse) targets **logged-in `deskflow-core`** — not the LoginWindow LaunchAgent.

Investigation should follow the bridge’s real dependency chain before writing new code.

---

## What the login bridge needs (unchanged design)

```
Remote Mac (elected server, deskflow-core listening)
        ▲ TCP (core/port, default 24800)
        │
Login Mac @ login window
  deskflow-vhid-bridge (LaunchAgent, LoginWindow session)
        ▼
  Karabiner-VirtualHIDDevice-Daemon (DriverKit)
        ▼
  Virtual keyboard + pointer → loginwindow
```

**Enable path (GUI):** Settings → Login Window → checkbox → admin prompt installs  
`/Library/LaunchAgents/org.deskflow.vhid-bridge.plist`  
**Takes effect only after logout or restart** (LoginWindow agents cannot hot-load from user session).

**Plist args (derived from existing settings):**

| Arg | Source |
|-----|--------|
| Server hosts (CSV) | `coordination/peers` (all non-self names/addresses) |
| Client screen name | `core/computerName` |
| Port | `core/port` |
| `--scale` | `coordination/loginBridgeScale` (default 4.0) |

**Karabiner prerequisites:**

- Driver package installed + system extension approved
- `Karabiner-VirtualHIDDevice-Daemon` running
- Bridge binary at `{Deskflow.app}/Contents/MacOS/deskflow-vhid-bridge`

---

## What changed during consolidation (relevant to login unlock)

| Change | Touches login bridge? | Risk to login unlock |
|--------|----------------------|----------------------|
| `fed317496` — sharing UI moved to Server Config | No (Login tab kept) | Low |
| `f64343c45` — VirtualHostTracker, HID decode sync | No | Low |
| `04b322638` — review fixes, HID `m_validKeys` | No bridge code | Low |
| `fbd080bb7` — per-binary codesign before bundle | **Yes (install)** | **Medium** — stale install may lack signed `deskflow-vhid-bridge` |
| `e4c7052bb` — QApplication auto-mode fix | Indirect | **Medium** — improves server availability for bridge to connect |
| Accessibility gate (`ensureAccessibilityPermission`) | Indirect | **Medium** — blocks **server Mac** Start until TCC granted; bridge needs a listening server |
| `dacddbcf3` — SMAppService start-at-login | Indirect | Low — GUI at user login ≠ LoginWindow bridge |
| Deleted local `claude/karibeener-*` branch | No | None — fixes already on `master` |
| HID passthrough seize | No at login window | None (different session) |

**Conclusion:** No intentional login-bridge regression in source. Most likely causes are **operational** (reinstall, agent not re-applied, logout not done, Karabiner daemon, peers/server down at login).

---

## Latent settings bug (not current strip, but should fix)

`coordination/loginBridgeEnabled` and `coordination/loginBridgeScale` are **not** in `Settings::m_validKeys`. They survive today because `coordination` is **not** in `m_validGroup`. Same class of bug as pre-fix HID passthrough keys — should be hardened in a fix pass.

No unit test covers login-bridge key persistence (unlike `hidPassthroughKeysSurviveCleanSettings`).

---

## Diagnostic playbook (do this before coding)

Run on the **Mac that should be unlocked at login** (login Mac):

```bash
# 1. Bridge binary present in installed app
ls -la /Applications/Deskflow.app/Contents/MacOS/deskflow-vhid-bridge

# 2. LaunchAgent installed
cat /Library/LaunchAgents/org.deskflow.vhid-bridge.plist

# 3. Karabiner daemon
pgrep -fl Karabiner-VirtualHIDDevice-Daemon

# 4. After logout to login screen — bridge log
sudo tail -100 /var/log/deskflow-vhid-bridge.log
```

Run on the **server Mac** (machine you control from):

```bash
# Core must be listening when login Mac is at login window
# (auto mode elected server, or fixed server role)
pgrep -fl deskflow-core
# Settings: coordination/peers must list the login Mac and reachable server address
```

**GUI check:** Settings → Login Window → status line (`LoginBridgeManager::statusText()`):

| Status | Meaning |
|--------|---------|
| Bridge binary missing | Reinstall Deskflow (`scripts/install-macos.sh`) |
| Karabiner driver not installed | Install VirtualHIDDevice package |
| Daemon not running | Start/approve Karabiner extension |
| Ready to enable | Driver OK, agent not installed |
| Active | Agent plist present — still need logout if just enabled |

**Log triage (high-signal lines):**

- `virtual HID device not ready` → Karabiner daemon
- `no server hosts given` / connection backoff → peers empty or plist stale
- `bad or missing server hello` → server not running or wrong port/screen name
- `connected to host` + `handshake complete` → bridge OK; look at input path

---

## Approaches to fix

### Approach A — Diagnose-first, zero code (Recommended)

Run the playbook above. Fix the failing layer only:

1. Reinstall Deskflow if bridge binary missing/unsigned
2. Re-enable Login Window bridge + admin install + **logout**
3. Fix Karabiner driver/daemon if status not Ready/Active
4. On server Mac: grant Accessibility, press Start, confirm `coordination/peers` and elected server

- **Pros:** Fast; matches “no bridge source changes” finding; avoids speculative code
- **Cons:** Doesn’t harden settings or improve UX if root cause is confusion
- **Best when:** Log shows env/install issue (most likely given no_log state)

### Approach B — Hardening + operator UX

After diagnosis, ship small fork fixes:

1. Add `loginBridgeEnabled` / `loginBridgeScale` to `m_validKeys` + unit test
2. Settings Login tab: “Re-apply agent” button when peers/port/scale change without re-save
3. Post-install hook: detect missing `deskflow-vhid-bridge`, warn in Login tab
4. `INSTALL.md` / Login tab reminder: “Logout required after enable”

- **Pros:** Prevents recurrence; clearer fleet ops
- **Cons:** Doesn’t fix broken runtime if Karabiner/server down
- **Best when:** Diagnostics show config drift or reinstall friction

### Approach C — Self-test + log surfacing in GUI

Add macOS-only “Test login bridge” that:

1. Runs bridge binary in dry-run against current peers (logged-in session, limited)
2. Surfaces last 20 lines of `/var/log/deskflow-vhid-bridge.log` in Settings
3. Validates plist matches current `coordination/peers` + `core/port`

- **Pros:** Best operator experience; faster than SSH
- **Cons:** More implementation; LoginWindow-only behavior hard to fully test from GUI session
- **Best when:** Repeated fleet support burden

---

## Key Decisions

- **Scope:** Login-window unlock only; desktop Mouser/HID is out of scope for this brainstorm
- **No bridge rewrite:** Source unchanged in consolidation; fix env/install first
- **Recommended path:** Approach A → B if logs show config/persistence issues
- **Server dependency:** Bridge requires a **listening peer server** at login time — Accessibility gate on server Mac can block this indirectly
- **Logout is mandatory** after enabling agent — not a regression, but easy to miss after reinstall

## Open Questions

- What does `/var/log/deskflow-vhid-bridge.log` show after a failed login attempt?
- Was Deskflow reinstalled after `fbd080bb7` (codesign order change)?
- Is the server Mac running `deskflow-core` (auto or server mode) when testing login unlock?
- Are `coordination/peers` populated on both machines with reachable LAN addresses?
- Did user enable Login Window bridge after last reinstall, and perform logout?

## Success Criteria

- From elected server Mac, user can move mouse/type password at login Mac’s login window
- Settings → Login Window shows **Active** with Karabiner daemon running
- Bridge log shows `handshake complete` and sustained connection during login attempt
- Survives reboot → login window → control works without manual steps beyond one-time enable
