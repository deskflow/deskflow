# Wayland Capture Repro Matrix

Scope: Deskflow Wayland EI input capture on Linux Wayland sessions, with portal-backed capture/session lifecycle in:

- `src/lib/platform/EiScreen.cpp`
- `src/lib/platform/PortalInputCapture.cpp`
- `src/lib/platform/PortalRemoteDesktop.cpp`
- `src/lib/base/EventTypes.h`
- `src/lib/deskflow/ServerApp.cpp`
- `src/lib/deskflow/ClientApp.cpp`

Notes:

- `ServerApp` and `ClientApp` select `EiScreen` when `deskflow::platform::isWayland()` is true and `WINAPI_LIBEI` is enabled.
- `EiScreen` owns the EI backend state and reacts to `EIConnected` / `EISessionClosed`.
- `PortalRemoteDesktop` owns remote-desktop portal setup and reconnects after session close.
- `PortalInputCapture` owns input-capture portal setup, activation, barriers, and re-enable timing.
- There do not appear to be capture-state unit tests under `src/unittests/platform/` yet.

| Scenario | Repro | Expected current bad behavior | Likely owner |
|---|---|---|---|
| Suspend / resume | Start Deskflow on Wayland, begin a capture session, suspend the machine, then resume and return to the same session. | Primary-side input capture can be disabled or lose its active state across suspend, and `EiScreen` will rebuild EI state when it sees `EISessionClosed`. The user-visible symptom is still a stalled or inactive capture path until the portal/session comes back. | `EiScreen.cpp`, `PortalRemoteDesktop.cpp`, `PortalInputCapture.cpp` |
| Logout / login or screen unlock | Start a capture session, log out and back in, or lock the screen and unlock it. | Primary-side input capture and secondary-side remote-desktop reconnection are handled by different portal objects. On logout/unlock, the remote-desktop portal can close and reconnect, while input capture may drop its active session and require re-arming before input flows again. | `PortalRemoteDesktop.cpp`, `PortalInputCapture.cpp`, `EiScreen.cpp` |
| Monitor unplug / replug | While capture is active, unplug a monitor, then plug it back in, or otherwise change the monitor layout. | Primary-side input capture rebuilds barriers on `zones-changed`, but that does not guarantee the active capture session stays valid. The likely symptom is a temporary loss of capture or a stale activation until the portal settles. | `PortalInputCapture.cpp`, `EiScreen.cpp` |

Observed failure shape to look for during later repro runs:

- input stops moving between screens even though Deskflow is still running
- portal session closes and capture does not re-arm automatically
- cursor stays trapped or capture must be manually restarted
