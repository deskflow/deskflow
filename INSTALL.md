# Installing this fork across your devices

This guide is for building and installing **this fork** of Deskflow
(`hughesyadaddy/deskflow`, branch **`master`**) on each of your machines.

## What this fork adds

- **Native auto-switch:** in-process role election (`deskflow-core auto`) replaces
  external `coordinator.py` / `KvmSwitch.exe` supervisors. See `docs/coordination/`.
- **HID passthrough:** relay raw vendor HID reports to the focused machine.
  See `docs/hid-passthrough.md`.
- **Mouser bridge:** Logitech HID++ gesture relay via loopback JSON.
  See `docs/mouser-bridge.md`.
- **macOS onboarding:** pressing **Start** checks Accessibility permission first
  (`MainWindow.cpp` → `ensureAccessibilityPermission`).
- **macOS login bridge (optional):** Karabiner-based login-window injection for
  auto-switch on the login screen. Configured in Settings → Login bridge; not
  required for normal desktop use.
- **Fleet install scripts:** `scripts/install-macos.sh`, `scripts/install-windows.ps1`.

> Day-to-day KVM does **not** require Karabiner-Elements. The optional login bridge
> uses Karabiner DriverKit only for login-screen injection.

---

## Step 0 — Keep the fork up to date with upstream

Run periodically from a machine with GitHub access:

```bash
git remote add upstream https://github.com/deskflow/deskflow.git   # first time only
git fetch upstream
git checkout master && git merge --ff-only upstream/master && git push origin master
```

Track upstream PR progress in `docs/FORK_ROADMAP.md`.

## Step 1 — Get the code on each device

```bash
git clone https://github.com/hughesyadaddy/deskflow.git
cd deskflow
git checkout master
```

## Step 2 — Build & install

Requirements (all platforms): CMake 3.24+, Qt 6.7+, OpenSSL 3.0+.
(`google_test` is fetched automatically if missing.)

### macOS (including a macOS VM on Proxmox)

```bash
# Toolchain + deps via Homebrew
brew install cmake qt openssl@3

# Configure, build, and install to /Applications (quits, deploys, restarts)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
bash scripts/install-macos.sh
# or: bash scripts/install.sh
```

`scripts/install-macos.sh` quits Deskflow (GUI, deskflow-core, deskflow-vhid-bridge), stages a
`cmake --install` (macdeployqt), copies only `Deskflow.app` to `/Applications`, clears
quarantine, and relaunches from the install path.

Manual copy (same end state without macdeployqt):

```bash
cp -R build/bin/Deskflow.app /Applications/
xattr -c /Applications/Deskflow.app    # clear quarantine on unsigned local builds
```

Then launch Deskflow and press **Start**. When prompted, grant **Accessibility**
to **both** `Deskflow` and the `deskflow` process under
System Settings → Privacy & Security → Accessibility. On macOS Sequoia, also allow
**Local Network**.

### macOS login bridge (optional, after install)

Reinstalling Deskflow (`scripts/install-macos.sh`) updates the app bundle but **does
not** install the login-window LaunchAgent. After every fresh install:

1. Install [Karabiner DriverKit VirtualHIDDevice](https://github.com/pqrs-org/Karabiner-DriverKit-VirtualHIDDevice/releases) and approve the system extension.
2. Configure `coordination/peers` on all cluster machines (use `name=ip` forms if hostnames fail at login).
3. Deskflow → **Settings → Login Window** → enable **Control this computer at the login window** → approve admin prompt.
4. **Log out or restart** (LoginWindow agents do not hot-load from a user session).
5. Confirm status shows **Active**; use **Refresh** on the bridge log if troubleshooting (`/var/log/deskflow-vhid-bridge.log`).
6. From the elected server Mac (with core running), test pointer and password at the login screen.

### Linux / BSD

Additional deps: `libei` 1.3+ and `libportal` 0.9.1+ (install from your package
manager, e.g. `libei-dev`, `libportal-dev`).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build
```

### Windows

Use the Qt online installer (or `-DVCPKG_QT=ON`), then:

```bat
cmake -S . -B build
cmake --build build --config Release
pwsh scripts\install-windows.ps1
```

`scripts/install-windows.ps1` stops the Deskflow service and all deskflow processes, runs
`cmake --install` into `C:\Program Files\Deskflow` (elevates if needed), registers/updates the
Windows service, and launches `deskflow.exe` from that directory.

Build + install in one step: `pwsh scripts\build-windows.ps1 -Install`

## Step 3 — Optional: build installable packages

```bash
cmake --build build --target package        # dmg (macOS), deb/rpm/archive (Linux), etc.
```

## Developer workflow (VS Code / Cursor)

Debug builds use `build-debug/` and never overwrite `/Applications` by default.

1. Copy launcher config (`.vscode/` is gitignored):
   ```bash
   mkdir -p .vscode
   cp docs/dev/launch.json.example .vscode/launch.json
   cp docs/dev/tasks.json.example .vscode/tasks.json
   ```
2. Install the **CodeLLDB** extension on macOS.
3. Read `docs/dev/MACOS_DEBUG.md` for GUI vs core vs attach workflows.

---

## If it crashes — collect this and send it

```bash
# macOS crash report (preferred — has the faulting line):
cat "$(ls -t ~/Library/Logs/DiagnosticReports/Deskflow* 2>/dev/null | head -1)"

# Deskflow's own log (set log level to DEBUG in Settings first):
tail -200 ~/Library/Logs/Deskflow/*.log
```

Paste the output back so the crash can be pinpointed and patched on `master`.
