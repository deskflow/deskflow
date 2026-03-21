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
| Suspend / resume | Start Deskflow on Wayland, begin a capture session, suspend the machine, then resume and return to the same session. | EI/portal state can drop across suspend. Current code may recreate EI state only after disconnect, and capture can remain inactive or require a full restart before input works normally again. | `EiScreen.cpp`, `PortalRemoteDesktop.cpp`, `PortalInputCapture.cpp` |
| Logout / login or screen unlock | Start a capture session, log out and back in, or lock the screen and unlock it. | Portal session closure is treated as a hard lifecycle event. The session may close and reconnect, but capture can be lost during the transition and may not recover cleanly without restarting Deskflow. | `PortalRemoteDesktop.cpp`, `PortalInputCapture.cpp`, `EiScreen.cpp` |
| Monitor unplug / replug | While capture is active, unplug a monitor, then plug it back in, or otherwise change the monitor layout. | `zones-changed` handling rebuilds barriers and re-enables capture, but active capture can still be dropped or left in a stale state until the portal/session settles. | `PortalInputCapture.cpp`, `EiScreen.cpp` |

Observed failure shape to look for during later repro runs:

- input stops moving between screens even though Deskflow is still running
- portal session closes and capture does not re-arm automatically
- cursor stays trapped or capture must be manually restarted

