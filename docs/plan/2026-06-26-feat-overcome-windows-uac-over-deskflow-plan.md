# feat: drive Windows UAC prompts over deskflow without breaking the KVM mesh

**Date:** 2026-06-26
**Type:** enhancement (Windows deployment / config; optional core change)
**Target machine:** tiny11 (Windows 11, VID/PID NuPhy aside) — the deskflow client/auto node
**Status:** ✅ RESOLVED — implemented **auto-elevate** in the fork (supersedes Option B)

> **Final resolution (2026-06-26):** Rather than weaken UAC (Option B silent-elevate, applied then reverted), we implemented the *proper* fix in the fork's Windows daemon: **auto-elevate**. The watchdog now runs the core at the user's (MEDIUM) integrity on the normal desktop — so PowerToys/Mouser hook it — and relaunches it **SYSTEM only while the secure/login desktop is active**, detected by `consent.exe` (UAC) / `LogonUI.exe` (lock/login) presence (session-0-safe). Commits: `a6274ca15` (auto-elevate) + `e80a25658` (latency fix, ~4s). Verified on tiny11: medium at rest → SYSTEM in ~4s when a UAC prompt appears → clickable → back to medium. `ConsentPromptBehaviorAdmin` restored to `5` (full UAC on). This gives **PowerToys + Mouser + login-screen + clickable UAC simultaneously** — the either/or constraint is gone. See `src/lib/platform/MSWindowsWatchdog.cpp` (`secureDesktopActive()`, `mainLoop`, `startProcess`).
>
> _The analysis below is retained as the investigation record; its Option-A/B/C/D framing led to the auto-elevate (Option C done right) outcome above._

---

## 1. Problem

When controlling **tiny11** remotely through deskflow, you cannot interact with **UAC consent prompts** (the "Do you want to allow this app to make changes?" dialog). The mouse/keyboard go dead on that dialog even though normal control works.

The cause is the **UAC secure desktop**. When a UAC prompt fires, Windows switches input to an isolated desktop (`Winlogon\Winlogon` secure desktop) that only SYSTEM/trusted processes may touch, specifically to stop *programmatic* (synthetic) input — which is exactly what deskflow injects (`SendInput`/`SetCursorPos`). So the prompt is unreachable.

## 2. Hard constraint — must NOT break any of this

The setup was just painstakingly restored. Any approach **must leave all of the following working**:

| Capability | How it works today | Don't disturb |
| --- | --- | --- |
| **Login-screen injection** | Daemon (`Deskflow` service, SYSTEM) auto-arms from `daemon/configFile` and runs `deskflow-core auto … --settings <user conf>` **elevated** at boot | The daemon arming + elevated SYSTEM core |
| **Mouser gestures** | `client/mouserToken=0795…` matches Mouser `remote_device` (19795); `mouserBridgeToken=a9e8…` matches `remote_forward` (19796) | The two token values |
| **PowerToys** | Hook-based modules receive the SYSTEM core's injected input (medium-integrity modules see LL-hook input) | Core integrity / session |
| **Coordination / auto-switch** | `coordination/peers` (macbookpro/hackintosh/tiny11, `.106` fix) + `coordination/port=24851` in the user conf the SYSTEM core reads | The peers + the `--settings` path |

**Design rule:** prefer solutions that **touch only Windows policy/registry, not deskflow**, because the deskflow Windows path (daemon → watchdog → desktop-following core) is the fragile, hard-won part.

## 3. Goals & non-goals

**Goals**
- Be able to click/type on UAC consent prompts on tiny11 while driving it from macbookpro.
- Zero regression to login-screen, Mouser, PowerToys, coordination.
- Instant, documented rollback.

**Non-goals**
- Disabling UAC entirely (`EnableLUA=0`) — breaks modern/Store apps, requires reboot, massive blast radius. Out of scope.
- Solving this for hackintosh/macOS (UAC is Windows-only).
- Making the secure desktop itself injectable in the general case (only as an optional, higher-risk path — see Option C).

## 4. Background — why the SYSTEM core *almost* already does this

Two facts from the codebase make the policy approach safe:

1. **The core already follows the active input desktop.** `MSWindowsScreen::updateDesktopThread()` (`src/lib/platform/MSWindowsScreen.cpp:484`) does:
   ```cpp
   HDESK hDesk = OpenInputDesktop(0, true, GENERIC_ALL);  // whatever desktop has input now
   SetThreadDesktop(hDesk);                                // move our input thread onto it
   ```
   Running as SYSTEM, this can attach to high-integrity desktops the user session cannot.

2. **The watchdog already runs the core elevated with the winlogon token** (`src/lib/platform/MSWindowsWatchdog.cpp:134-157`, `getUserToken(..., elevatedToken=true)` duplicates `winlogon.exe`'s token). That SYSTEM/Session-1 core is what's running right now (pid observed: `deskflow-core.exe … auto --settings …`, owner `NT AUTHORITY\SYSTEM`).

So the only thing standing between today's working SYSTEM core and the UAC dialog is the **secure-desktop switch**, which moves the prompt onto a desktop the core isn't notified to follow. Remove that switch and the existing core reaches the prompt on the normal desktop. **No core change required.**

## 5. Options

### Option A — Disable the UAC *secure desktop* (recommended primary)
Keep UAC prompts, but render them on the **normal interactive desktop** instead of the isolated secure desktop. The already-running SYSTEM core then injects into them like any other window.

- **Knob:** `HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System\PromptOnSecureDesktop = 0` (DWORD).
- **Touches deskflow?** No. Pure Windows policy.
- **Effort:** one registry value; effective immediately (no reboot for the prompt-rendering change).
- **Risk to the mesh:** none — deskflow untouched.
- **Security trade-off:** the consent UI is no longer isolated, so *local* malware could synth-click it. Acceptable for a personal homelab node behind Tailscale; document it.

### Option B — Reduce/eliminate the prompt for admins
Change *when* UAC prompts at all.
- `…\Policies\System\ConsentPromptBehaviorAdmin`: `5` = prompt for consent on secure desktop (default), `2` = consent on secure desktop, `0` = **elevate silently** (no prompt).
- Setting `0` makes admin elevation silent → nothing to click. Bigger security reduction than A (any admin-requesting app auto-elevates). Touches deskflow? No.
- Use only if you'd rather have *no* prompt than a clickable one.

### Option C — Make deskflow follow the secure desktop (keep it enabled) — *higher risk, optional*
Keep the secure desktop's protection and teach the core to switch onto it when active.
- The hook exists (`updateDesktopThread`), but it must be (a) **triggered** when the secure desktop activates (desktop-switch / `WTS`/`WM_WTSSESSION_CHANGE` is not the same signal as a same-session secure-desktop switch), and (b) allowed to `SetThreadDesktop` to `Winlogon\Winlogon` (SYSTEM token can; needs the right `OpenDesktop` flags), and (c) re-inject via the watchdog's elevated path.
- **Touches the fragile core/watchdog → real regression risk** to login-screen/mesh. Treat as a *future* item only if Option A's security trade-off is unacceptable.
- Files in play: `MSWindowsScreen.cpp` (`updateDesktopThread`, desktop-change events ~lines 459/477), `MSWindowsWatchdog.cpp` (session/desktop loop ~lines 406/498).

### Option D — Per-app auto-elevation (no global UAC change) — good complement
For the handful of apps that prompt, remove *their* prompt without weakening global UAC:
- Create an **elevated Scheduled Task** (`/rl HIGHEST`, `/ru alexh /it`) that launches the app, plus a normal shortcut that runs `schtasks /run …`. The task runs elevated with no consent prompt.
- **Touches deskflow?** No. Surgical, keeps the secure desktop on for everything else.
- More setup per app; only worth it for a small, known set.

## 5b. Research findings (2026-06-26, sourced) — the *proper* way

Web + source research changed the recommendation. Key facts:

- **You cannot click a UAC consent prompt with ordinary injected input** from the wrong desktop or below SYSTEM. UIPI + the secure desktop block it by design (MS UAC docs; Google Project Zero "Bypassing Administrator Protection by Abusing UI Access"). `PromptOnSecureDesktop=0` alone does **not** make it clickable — confirmed empirically on tiny11 (it was already `0`).
- **How remote tools actually do it:** they run an **input injector as a SYSTEM service** and operate **on the secure desktop**. AnyDesk docs: *"When the AnyDesk Service is installed, it can interact with … UAC elevation requests."* TeamViewer/AnyDesk show a black/dimmed screen on the secure desktop but **input works once the service is on that desktop**.
- **deskflow's own design matches this** (Synergy "make Synergy work with UAC prompts" + deskflow issue [#8183](https://github.com/deskflow/deskflow/issues/8183), CLOSED/COMPLETED): set elevate to **"Always"**, and the **daemon relaunches the elevated SYSTEM core on the active desktop, including the secure desktop**. #8183 was a regression in *active-desktop detection* (`getActiveDesktop`/`--get-active-desktop`); the fix restored relaunching the elevated core on the correct desktop.
- **The Windows-sanctioned app mechanism is UIAccess** (manifest `uiAccess=true` + Authenticode-signed + installed under `Program Files`) to drive input to higher-privilege windows — but consent.exe runs at **SYSTEM IL**, so UIAccess alone isn't sufficient for the consent prompt; the **SYSTEM-on-secure-desktop** path is what's needed.

**Fork mechanism check:** the watchdog relaunches the core on `m_session.hasChanged()` (`MSWindowsWatchdog.cpp:180`, session-level) and has `ArchMiscWindows::getActiveDesktopName()`. The **login screen works** for tiny11 because Winlogon trips the session/desktop check. The **UAC secure desktop is a desktop switch *within* Session 1**, which `hasChanged()` may not catch → the elevated core isn't relaunched onto the secure desktop → prompt unreachable. **That's the real, debuggable gap** (the deskflow-side path, not a UAC policy).

## 6. Recommendation — UPDATED after apply-time finding + research

> **Finding (2026-06-26, during apply):** `PromptOnSecureDesktop` was **already `0`** on tiny11 (snapshot: `PromptOnSecureDesktop=0, ConsentPromptBehaviorAdmin=5, EnableLUA=1`), yet UAC prompts still can't be clicked. **Conclusion: Option A is insufficient.** The UAC consent UI (`consent.exe`) is a *protected* process that ignores synthetic input even on the normal desktop and even from SYSTEM — by design. You cannot reliably *click* a UAC prompt via injection. The only reliable way to "overcome" it is to **remove the prompt** for the operations you care about.

Revised layering — **keep UAC on; make deskflow follow the secure desktop (the proper way)**:

1. **Primary: Option C′ — "Always elevate" + secure desktop ON + working desktop-follow.** This is deskflow's *designed* UAC path and how AnyDesk/TeamViewer do it.
   - Set `daemon/elevate = true` ("Always", not "auto") — **already set** on tiny11.
   - **Restore `PromptOnSecureDesktop = 1`** (it was wrongly set to `0`). deskflow is built to *follow onto* the secure desktop; the prompt must be there for the relaunch-on-switch to engage.
   - Verify/fix the daemon's **desktop-switch follow**: when UAC fires, the daemon must relaunch the elevated SYSTEM core on the secure desktop. If it doesn't (because `hasChanged()` only catches session-level changes, not the in-session secure-desktop switch), that's the deskflow fix to make — extend the watchdog to detect the secure-desktop switch (compare `getActiveDesktopName()` against the running core's desktop) and relaunch there. Reference issue #8183.
   - Expected UX: brief black/dimmed remote view on the secure desktop (same caveat as AnyDesk/TeamViewer), then you can click Yes/No.
2. **Fallback if you don't want to touch deskflow code: Option B — `ConsentPromptBehaviorAdmin=0` (silent elevate).** No prompt to click at all. Security trade-off: any admin request elevates without consent (§11). Reversible to `5`.
3. **Surgical alternative: Option D — per-app elevated Scheduled Tasks.** Keeps full UAC; removes prompts only for chosen apps.
4. **Reject `EnableLUA=0`** — breaks Store/modern apps, needs reboot, huge blast radius.

> Net: the user's instinct ("can't I just accept the prompt from desktop mode?") is **correct and is the proper path** — via always-elevate + secure-desktop-following, not by disabling UAC. The work, if any, is on the **deskflow desktop-follow**, not on weakening Windows security.

## 7. Implementation steps (Option A, primary)

> All commands run over SSH (no login needed) and are validated against the live state — nothing here touches `C:\Program Files\Deskflow` or any deskflow config.

```powershell
# 7.1 Snapshot current value (for rollback)
$k = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System"
Get-ItemProperty $k -Name PromptOnSecureDesktop, ConsentPromptBehaviorAdmin, EnableLUA |
  Format-List   # record these

# 7.2 Disable the secure desktop for prompts (keep UAC consent itself)
Set-ItemProperty $k -Name PromptOnSecureDesktop -Type DWord -Value 0

# 7.3 No reboot needed for prompt rendering; confirm
(Get-ItemProperty $k -Name PromptOnSecureDesktop).PromptOnSecureDesktop   # -> 0
```

**Rollback (one line):**
```powershell
Set-ItemProperty "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System" `
  -Name PromptOnSecureDesktop -Type DWord -Value 1
```

### Per-app task pattern (Option D, if used) — mock: `elevate-<app>.ps1`
```powershell
# create once per app
schtasks /create /tn "Elev_<App>" /tr '"C:\Path\To\App.exe"' /sc once /st 23:59 `
  /ru alexh /rl HIGHEST /it /f
# launch with no prompt
schtasks /run /tn "Elev_<App>"
```

## 8. Flow / edge-case analysis

| Scenario | Expected with Option A | Verify |
| --- | --- | --- |
| UAC prompt while you're driving tiny11 from macbookpro | Prompt renders on normal desktop; SYSTEM core clicks it | manual: trigger an installer/elevation, click Yes remotely |
| UAC prompt at the **login screen** (rare) | Login screen unaffected (different mechanism); SYSTEM core already handles Winlogon desktop | trigger at lock screen if applicable |
| Reboot | `PromptOnSecureDesktop=0` persists (HKLM); daemon re-arms SYSTEM core; both independent | reboot, re-test prompt + login screen |
| Mid auto-switch (focus flips to tiny11 as a prompt appears) | Coordination unaffected; core follows input desktop as today | switch in, trigger prompt |
| Mouser gesture during/after a prompt | Unaffected — token path untouched | gesture test |
| Rollback | Secure desktop returns; remote UAC control lost again, everything else identical | apply rollback, confirm |

Edge cases to call out in testing: a UAC prompt that appears **before** the core has attached to the current desktop (race) — the core's `updateDesktopThread` should re-attach on the next input-desktop change; if a prompt is ever unreachable, nudging focus (switch out/in) re-triggers attach.

## 9. Risks & mitigations

| Risk | Likelihood | Mitigation |
| --- | --- | --- |
| Security: local malware auto-clicks consent | Low (single-user homelab, Tailscale-gated) | Accept + document; keep machine otherwise locked down; can scope with Option D instead |
| Registry change mis-applied | Low | Snapshot first (7.1); one-line rollback (7.2) |
| Someone assumes deskflow changed | — | This option **does not touch deskflow**; note it in the runbook |
| Option C attempted and destabilizes core | Med (if pursued) | Keep C out of scope; if ever done, branch off, test login-screen + mesh before merge |

**Regression gate (run after ANY change here):** login-screen core present (`deskflow-core … owner SYSTEM`), Mouser handshake non-`unauthorized` once logged in, a Logi gesture relays, coordination shows `following "macbookpro"`. If any fail → rollback.

## 10. Acceptance criteria

- [ ] From macbookpro, a UAC consent dialog on tiny11 can be clicked/typed (Yes/No/cancel) remotely.
- [ ] `deskflow-core.exe` still runs as `NT AUTHORITY\SYSTEM` and the login screen is still drivable.
- [ ] After login, Mouser logs **no** `unauthorized`; a gesture relays.
- [ ] Coordination still reports `following "macbookpro"`; auto-switch works.
- [ ] `PromptOnSecureDesktop` documented with its prior value and a tested one-line rollback.
- [ ] No file under `C:\Program Files\Deskflow\` or any `Deskflow.conf` was modified by this change.

## 11. Security considerations (explicit)

Disabling the secure desktop removes UAC's anti-automation isolation: the consent prompt becomes injectable by any process that can reach the interactive desktop. On a single-user homelab node reachable only over Tailscale, the practical exposure is low and the remote-admin benefit is the whole point. If that trade-off is unwanted, use **Option D** (per-app elevated tasks) and leave the secure desktop on globally.

## 12. Open questions

- Which apps actually trigger the UAC prompts you care about? (If it's a small, known set, Option D may beat Option A and keep full security.)
- Do you ever need to approve a UAC prompt **at the login screen / before logging in**, or only within your session? (Affects whether the Winlogon-desktop path matters.)
- Acceptable to apply Option A fleet-wide eventually, or tiny11 only?

## 13. References

- `src/lib/platform/MSWindowsScreen.cpp:484` — `updateDesktopThread()` (input-desktop following).
- `src/lib/platform/MSWindowsWatchdog.cpp:134` — `getUserToken(elevatedToken)` (winlogon token, elevated launch).
- `src/apps/deskflow-daemon/DaemonApp.cpp:57,178` — `applyWatchdogCommand()` + `daemon/configFile` re-arm (the login-screen mechanism this plan must not disturb).
- Windows UAC policy keys: `HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System\{PromptOnSecureDesktop, ConsentPromptBehaviorAdmin, EnableLUA}`.
