# Deskflow Smooth Mode

Automatic AWDL suppression only while you control the Deskflow client, with AirDrop allowed when you return to the server.

The input fixes in this fork run inside Deskflow. This optional companion handles only AWDL because interface changes require administrator privileges. Install it on **both Macs**, choosing the correct server/client role. It follows each Mac's ordered Deskflow Info log; it adds no network control service.

Build the package with `-DBUILD_MACOS_SMOOTH_MODE=ON`, or run `/bin/zsh "Build from source.command"` in this source directory. CMake places a complete package under `build/tools/macos-smooth-mode/`. Building and running the tests do not install services or change interfaces.

## Behavior

| Deskflow focus | Server AWDL | Client AWDL |
|---|---|---|
| Server active | Allowed/restored | Allowed/restored |
| Client active | Suppressed | Suppressed |
| Focus unknown or Deskflow stops | Suppression released | Suppression released |

During client control the worker watches interface changes and puts `awdl0` down again if macOS/AirDrop re-enables it. On return to the server it restores an UP state that it suppressed. It does not force an originally DOWN interface UP merely because focus is unknown; AirDrop is then free to enable it as needed.

**AirDrop and suppression cannot operate simultaneously on the same interface.** Start AirDrop while the server has focus, and remain there during the transfer. If you need to control the client during a transfer, choose **DF → Allow AirDrop for 10 minutes on both Macs** first. That override temporarily permits AWDL at the cost of possible cursor lag. It expires automatically; choose Resume automatic mode to finish it sooner.

## Administrator setup

This requires one administrator installation on each Mac because macOS restricts interface flag changes. The privileged component has a fixed interface name, `awdl0`; it uses kernel interface ioctls. It has no shell-command facility, network listener, input capture, setuid bit, or sudoers change.

Read `awdl-guard.c`, `smooth_policy.h`, `SmoothMenu.m`, and `SmoothSetup.m` to review the complete source. These are local, ad-hoc signed builds, not Apple-notarized releases. The standalone build script produces Apple Silicon and Intel binaries; CMake uses the selected build architecture. macOS 13+ is required.

1. Extract the whole folder on each Mac.
2. On the server, run **Install Server.command**. On the client, run **Install Client.command**. Terminal asks for the administrator password locally; it is not sent to this chat.
3. Deskflow briefly restarts. The DF menu-bar control starts automatically.
4. After both installations, switch to the client and back once using Shift+Tab. This initializes focus tracking from fresh transitions.

Alternatively, from the extracted folder:

```sh
# Server only
sudo ./smooth-setup install server
```

```sh
# Client only
sudo ./smooth-setup install client
```

### Download verification warning

AirDrop/downloads may quarantine these local builds. They are not Apple-notarized. Apple documents a per-app **Open Anyway** option under [Privacy & Security](https://support.apple.com/en-us/102445). For this locally generated package, from its extracted directory, you can instead clear quarantine only from the three executable components before installation:

```sh
xattr -dr com.apple.quarantine ./smooth-setup ./awdl-guard "./Deskflow Smooth Mode.app"
```

Then rerun the appropriate installer command above. This does not disable Gatekeeper or change verification policy for other apps. Use this only for this package that you copied from the server.

The installer refuses a role that differs from Deskflow's saved mode. It preserves all hotkeys, cursor lock, computer layout, protocol, TLS, and VPN settings. It enables a dedicated **Info-level Deskflow file log**, retaining a backup and changing only the `log/file`, `log/toFile`, and `log/level` preferences. A preferences backup is kept in `~/Library/Application Support/DeskflowMXAWDL/`.

## Controls

Deskflow also exposes these controls under **Deskflow's menu-bar icon → Smooth Mode**, with an automatic smoothing toggle and live status under **Preferences → macOS**. Preferences applies an explicit change on Save and leaves it unchanged on Cancel. It communicates with the same installed worker. You can unload the separate DF menu agent while using the integrated controls; do not uninstall the worker or disable its file logging. The legacy installer still installs the standalone DF menu for users of the official Deskflow build.

The **DF** menu bar icon offers:

- **Automatic smoothing**: toggle the automatic controller. Turning it off permits AWDL.
- **Allow AirDrop for 10 minutes**: temporarily override suppression on this Mac. Use it on both Macs when needed during client control.
- **Resume automatic mode now**: end the override early.
- **Show controller log**.
- **Quit controls and allow AirDrop**: disable automation and close the menu. The preference remains off until re-enabled.

The menu app runs as your normal user and needs no Accessibility permission. A small fixed-purpose root worker performs the interface changes. It tracks Deskflow-core exit/restart, log replacement/truncation, and disconnect messages. Unknown state releases suppression rather than assuming the client remains active.

## Installed locations

```text
/Library/PrivilegedHelperTools/local.deskflow.mx-awdl
/Library/LaunchDaemons/local.deskflow.mx-awdl.plist
/Library/Logs/DeskflowMXAWDL/guard.log
/Library/Logs/DeskflowMXAWDL/status
~/Applications/Deskflow Smooth Mode.app
~/Library/LaunchAgents/local.deskflow.mx-awdl-menu.plist
~/Library/Application Support/DeskflowMXAWDL/control
~/Library/Logs/DeskflowMXAWDL/deskflow.log
```

The root worker and daemon configuration are root-owned. The user control file permits only automatic/off/AirDrop-pause state; it cannot specify an interface or command. Log files are parsed only for fixed focus/lifecycle messages, never executed.

## Remove

Run **Uninstall Smooth Mode.command** on each Mac, or:

```sh
sudo ./smooth-setup uninstall
```

The worker releases its AWDL suppression before stopping. The installer removes its app and launch entries and restores the original three Deskflow logging preferences. Other current Deskflow settings are retained. Diagnostic logs and the preferences backup remain available. The native Deskflow input fixes are separate and are unaffected.

## Validation and limits

The binaries compile without warnings. Simulated integration tests passed for both roles: historical-log rejection, focus direction, unexpected AWDL re-enable, server return, AirDrop override and expiry, automatic disable, log rotation, process exit/restart, disconnect, symlink rejection, and shutdown restoration. All simulation tests use fake interface state and do not change actual networking. Scoped preference-edit/restore tests passed too.

Real privileged interface switching and the two-Mac behavior must still be checked after administrator installation. macOS can delay notifications or refuse an interface change; the menu and logs expose the actual state/error. File logging must stay enabled. After a log replacement or helper/core restart, a fresh switch cycle may be needed to establish focus.

An in-place same-size rewrite of a log without observable truncation is outside the follower's guarantees. Use the dedicated append log with normal rotation. A forced kill of the privileged worker can delay restoration until launchd restarts it; a saved owned-state record supports recovery and remains owned until restoration succeeds, including after temporary interface errors. Unrelated aborted server-side handshakes do not reset the active-client focus state. A graceful stop restores the UP state it suppressed.
