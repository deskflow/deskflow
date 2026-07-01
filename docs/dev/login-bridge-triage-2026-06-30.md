# Login bridge triage — hackintosh — 2026-06-30

## Symptom

Cannot control/unlock at login window from another cluster machine.

## Phase 0 results (this machine: hackintosh)

| Check | Result |
|-------|--------|
| `deskflow-vhid-bridge` in app bundle | ✅ `/Applications/Deskflow.app/Contents/MacOS/deskflow-vhid-bridge` |
| LaunchAgent plist | ❌ **Missing** `/Library/LaunchAgents/org.deskflow.vhid-bridge.plist` |
| Karabiner daemon | ✅ PID 286 running |
| Bridge log | ❌ No `/var/log/deskflow-vhid-bridge.log` (agent never ran) |
| `deskflow-core auto` | ✅ Running |
| `coordination/peers` | `macbookpro, hackintosh, tiny11` |
| `coordination/loginBridgeEnabled` | ❌ **Not in Deskflow.conf** |

## Root cause

**Not a consolidation code regression.** The login-window LaunchAgent was never installed (or was removed). Without the plist, `deskflow-vhid-bridge` does not run at the login screen — Karabiner virtual HID is never fed.

Consolidation commits did not modify `LoginBridgeManager` or `deskflow-vhid-bridge` source. Reinstall (`scripts/install-macos.sh`) replaces the app bundle but does **not** install the LaunchAgent — user must re-enable in Settings after each fresh install.

## Fix (Phase 1 — operator)

1. Open Deskflow → **Settings** → **Login Window** tab
2. Enable **Control this computer at the login window**
3. Approve the admin prompt (installs plist)
4. **Log out or restart** (required — LoginWindow agents do not hot-load)
5. From elected server (e.g. macbookpro), test pointer + password at login screen
6. If still failing: `sudo tail -100 /var/log/deskflow-vhid-bridge.log`

## Secondary risk (after agent installed)

Peers are **hostname-only** (no `name=192.168.x.x` addresses). At login window, DNS/Tailscale may not resolve. If log shows connection failures, update peers:

```ini
peers=macbookpro=192.168.x.x|macbookpro.tail….ts.net, hackintosh=…, tiny11=…
```

Then re-save Login Window settings to refresh the plist.
