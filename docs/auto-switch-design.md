# Native auto-switch + login-screen injection — design

Goal: bake the "whichever machine you touch becomes primary" behavior (currently an
external coordinator that kills/relaunches `deskflow-core`) directly into Deskflow,
using its own patterns, plus a login-screen-capable injection path.

## Key findings from the codebase (grounding)

### Role is chosen once, but teardown is already clean
- `src/apps/deskflow-core/deskflow-core.cpp` `main()` picks `ServerApp` vs `ClientApp`
  once from args (`CoreArgParser`). `App` is a singleton (`App::s_instance`, asserted).
- `ServerApp`/`ClientApp` own a *mode-specific* `deskflow::Screen`, the TCP listener
  (server only, port 24800), and mode-specific event handlers; `mainLoop()` blocks in
  `getEvents()->loop()` until a `Quit` event.
- Clean teardown already exists: `ServerApp::cleanupServer()` / `ClientApp::stopClient()`
  release screen, listener, sockets, handlers.

**Implication:** a single process can't *morph* roles in place (singleton, immutable
screen `isPrimary`, exclusive listener, mode-specific handlers). BUT we don't need to —
we wrap `main()` in a **role-supervisor loop** that builds an App, runs its `mainLoop`,
and on a role-change intent posts `Quit`, lets the existing cleanup run, then builds the
other role — all **in-process** (no exec/relink). This reuses Deskflow's own teardown and
is dramatically faster than the current external kill+relaunch.

### Injection is cleanly behind interfaces (login-screen backend slots in here)
- `Client` → `deskflow::Screen` → `IPlatformScreen` (which is `ISecondaryScreen` +
  `IKeyState` + …). Injection surface: `fakeMouseMove/Button/RelativeMove/Wheel`
  (`ISecondaryScreen`) and `fakeKeyDown/Up/Repeat` (`IKeyState`).
- macOS today: `OSXScreen.mm` injects mouse via `CGEventPost`; `OSXKeyState.cpp` injects
  keys via `IOHIDPostEvent` (preferred) with `CGEventPost` fallback.
- A login-screen backend is a new `IPlatformScreen` (or just `IKeyState`) implementation —
  ~28 methods for a full screen, fewer if we subclass `OSXScreen` and swap only injection.

### **Surprise: Deskflow already has login-screen scaffolding**
- `OSXScreen.mm` carries the `__CGPreLoginApp,__cgpreloginapp` Mach-O section: *"tells
  macOS it is OK to let us inject input in the login screen."*
- It already prefers `IOHIDPostEvent` (the path that can survive at the login window).
- Entitlements (`src/apps/res/entitlements-dev.plist`, `deskflow.plist.in`) already include
  accessibility; codesigning via `cmake/MacCodesign.cmake`.

**Implication:** native Deskflow may drive the login screen via `IOHIDPostEvent` **without
the Karabiner virtual-HID bridge at all**, given correct signing/entitlements. That must be
tested first — if it works, the whole bridge component disappears. If modern macOS blocks
`IOHIDPostEvent` pre-login, we add a `VirtualHIDScreen` backend (DriverKit) behind
`IPlatformScreen` — same clean seam.

### Genuine-hardware detection has an exact recipe
- Capture is a `CGEventTap` in `OSXScreen::handleCGInputEvent()`; it filters by the
  `m_isOnScreen` state, NOT by event source.
- To know "a human physically touched THIS machine" (needed to auto-promote a client),
  check the source in the tap:
  `CGEventGetIntegerValueField(event, kCGEventSourceStateID)` →
  `kCGEventSourceStateHIDSystem` = real hardware vs injected. (Mirrors our `inputmon`.)

## Proposed architecture (3 components)

1. **Role supervisor** (`src/apps/deskflow-core/` + a small `RoleController`):
   outer loop owning the `EventQueue`; builds `ServerApp`/`ClientApp` per current role;
   a new `EventTypes::RoleChangeRequested` breaks `mainLoop` cleanly and re-enters as the
   other role. New CLI/config: `--auto-switch` + a peer list.

2. **Election/coordination layer** (`src/lib/deskflow/` new `AutoSwitchCoordinator`):
   tiny JSON-over-TCP mesh (port 24851) — the proven design from the external coordinator:
   CLAIM/heartbeat, sustained-input hysteresis, injection guard, address stickiness. A
   client that detects genuine local input claims primary; peers become clients of it.

3. **Login-screen injection**: first prove native `IOHIDPostEvent` works pre-login with
   proper signing; else add `OSXVirtualHidKeyState`/`VirtualHIDScreen` behind
   `IPlatformScreen` (DriverKit). Relative-pointer warp + accel handling already learned.

## Phased roadmap
- **P0 (proof):** sign deskflow-core with login entitlement; test if `IOHIDPostEvent`
  drives the login screen natively. Decides whether the bridge is even needed.
- **P1:** role-supervisor loop (in-process role flip) behind `--auto-switch`, no election
  yet (manual promote event). Proves clean in-process teardown/rebuild.
- **P2:** port the coordinator/election (component 2) into `AutoSwitchCoordinator`.
- **P3:** genuine-hardware tap-source detection → auto-promote.
- **P4:** if P0 failed, the `VirtualHIDScreen` backend; installer for the DriverKit ext +
  permissions (Accessibility/Input-Monitoring/system-extension approval) wizard.
- **P5:** Windows parity (the UIAccess + Raw Input pieces already designed).

## Licensing
Deskflow is GPLv2. A fork carrying these changes stays GPLv2; fine for personal use and
upstreamable as a feature (`--auto-switch`) if desired.
