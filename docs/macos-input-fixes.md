# macOS Options+ input compatibility

These optional features address mouse controls and modifier state when a macOS server shares a keyboard and mouse with another computer. They were developed and tested on a MacBook Pro server and Mac mini client using a Keychron keyboard and Logitech MX Master 3-family mouse. The Shift correction uses standard physical macOS modifier events; it does not add a Keychron-specific driver.

## Input and navigation

In **Preferences → macOS** on the server:

- **Logi Options+ input compatibility** forwards driver-injected middle clicks while controlling a client and synchronizes consumed physical Shift releases with local drivers. This addresses a middle-click Mission Control shortcut staying on the server and wheel scrolling being interpreted with stale Shift state after a Shift+Tab switch.
- **Back/Forward navigation gesture forwarding** forwards selected macOS swipe gestures as configurable client actions.
- **Disable screen-edge switching at startup** mirrors the existing cursor-lock preference. Configure your switch hotkey separately; Shift+Tab is a user choice, not a new default.

These settings also remain available under Configure Server → Advanced. They are disabled in client-mode Preferences. Direction detection and two configurable gesture directions are in the server Advanced dialog. Positive X defaults to Left / Action 1, and negative X to Right / Action 2.

Client settings provide Back, Forward, Ignore and custom keystrokes. Mac Back/Forward uses Command–[ / Command–]; other platforms use extra mouse buttons. Recorded Mac shortcuts translate Qt's Command/Control convention into physical Deskflow modifiers and pass through normal key handling, including secure-attention emulation where supported.

Native navigation requires protocol 1.9 on both ends. Earlier clients receive the extra-button fallback instead of the new navigation packet. Keyboard/layout packets retain their existing versioned formats.

Keep the desired button assignments in Options+, including application-specific profiles. For thumb-rest Mission Control, map the button to Middle click and assign Middle Mouse Button to Mission Control in client macOS settings. Deskflow forwards the event; it does not change that OS shortcut.

### Capture behavior

The normal HID capture tap remains in place. A session-stage tap handles other-button down/up events from `logioptionsplus_agent`. While controlling a client, eligible middle clicks are copied, tagged and reposted at HID for normal forwarding, consuming the session original. Tagged replacements cannot relay again. Ordinary physical HID clicks are already consumed while remote; local-server clicks are left unchanged.

When HID consumes a physical Shift release while remote, it also copies the release to SESSION so local drivers can clear their modifier state. Local releases, synthetic events, tagged copies, presses and releases with another Shift still held are excluded. The client receives the original through the normal keyboard path.

New input options are published as one lock-free atomic snapshot for the capture thread. Gesture conversion uses a per-callback autorelease pool. Both taps use the existing capture run loop and are removed on disable. A session tap timeout is re-enabled when Accessibility remains available; creation failure is logged. No additional typed-key listener or typed-character logging is introduced.

## Optional Wi-Fi smoothing

**Preferences → macOS → Automatic smoothing** and **menu-bar icon → Smooth Mode** control the same separately installed AWDL worker. This is an opt-in workaround for Wi-Fi interference associated with Apple's peer-to-peer interface, not a general network-latency fix.

During client control the worker suppresses `awdl0` on that Mac and resuppresses unexpected re-enabling. On return to the server or loss of known focus, it releases suppression and restores an UP state it owned. Enable it on both Macs if both need suppression. AirDrop/Continuity features using AWDL are unavailable during suppression. For transfers during client control, select **Allow AirDrop for 10 minutes** on both Macs; Resume ends the override early.

Preferences stages edits until Save. Cancel leaves the control unchanged; Reset reloads the current preference; Restore Defaults stages smoothing off. Saving an unrelated preference preserves an active AirDrop override. Status refreshes do not discard a pending edit. The checkbox is disabled when the controller is unavailable.

The optional package is built with `BUILD_MACOS_SMOOTH_MODE=ON`, which defaults off. Building it does not install services. Administrator installation is explicit and separate on each Mac. The worker can change only `awdl0`; it has no network listener, arbitrary command facility, input capture, setuid bit or sudoers change. It follows a dedicated Info-level Deskflow log and fixed control-file values. See the [component guide](../tools/macos-smooth-mode/README.md) for setup, permissions, recovery and removal.

## Build and validation

Use the normal upstream dependencies. One local Homebrew configuration was:

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DSKIP_BUILD_TESTS=ON \
  -DBUILD_INSTALLER=OFF -DBUILD_OSX_BUNDLE=OFF -DBUILD_MACOS_SMOOTH_MODE=ON \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qtbase);$(brew --prefix qttools)" \
  -DQt6LinguistTools_DIR="$(brew --prefix qttools)/lib/cmake/Qt6LinguistTools" \
  -DOPENSSL_ROOT_DIR="$(brew --prefix openssl@3)"
cmake --build build --target Deskflow deskflow-core MacPreferencesTests \
  MacSmoothModeMenuTests ServerConfigDialogTests ServerProxyTests ClientProxyTests \
  ComputerNavigationTests SettingsTests KeySequenceTests KeyStateTests \
  OSXInputCompatibilityTests OSXComputerTests macos-smooth-mode -j 6
ctest --test-dir build/src/unittests --output-on-failure \
  -R '^(MacPreferencesTests|MacSmoothModeMenuTests|ServerConfigDialogTests|ServerProxyTests|ClientProxyTests|ComputerNavigationTests|SettingsTests|KeySequenceTests|KeyStateTests|OSXInputCompatibilityTests|OSXComputerTests)$'
ctest --test-dir build/tools/macos-smooth-mode --output-on-failure
```

Fourteen selected suites passed locally: eleven native/protocol/settings/GUI suites and three helper suites. The environment was Apple Silicon, macOS 27.0.1, Qt 6.12.0, OpenSSL 3.6.3 and Apple Clang. Formatting with clang-format 20.1.0 and REUSE lint passed.

Regression coverage includes real in-memory hello/options/navigation flows for both protocol names and versions 1.6–1.9, legacy packet fallback, recorded modifiers, shortcut key-state restoration and secure-attention dispatch. Input tests construct or record events without posting them. GUI fixtures use temporary preferences and control files, covering shared Advanced settings, role restrictions, Save/Cancel/Reset/Defaults, external menu updates and AirDrop override preservation. Simulated helper tests cover both roles, crashes, failed restore retries, unrelated handshakes, process/log lifecycle, symlink rejection and restoration; they do not change real interfaces.

The owner reports successful live input and repeated switching tests with the native build on both Macs, with the old relay and DF menu stopped. Server diagnostics confirmed the fork/core, enabled settings, protocol 1.9 connection and AWDL restoration after returning. That deployment retained the earlier compatible AWDL worker; the revised worker's failure/recovery cases were tested in simulation. No Intel, Linux or Windows build, real Windows secure-attention delivery, privileged failure injection or reboot test is claimed here.

## Credits

Navigation capture and configurable direction mapping were adapted from Andy D.K Lee's [community macOS branch](https://github.com/andy5090/deskflow-andy-custom/tree/macos-logi-options-plus), including commits `3d59f` and `fb97e555baad7b4d23543a0c0e925fdbc77d3621`. Additional compatibility, client output, controls and regression coverage were prepared with OpenAI Codex assistance and owner testing.
