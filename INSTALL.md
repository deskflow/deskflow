# Installing this fork across your devices

This guide is for building and installing **this fork** of Deskflow
(`hughesyadaddy/deskflow`, branch `claude/karibeener-extension-install-mggwmq`)
on each of your machines over SSH.

## What this branch adds / preserves

- **macOS onboarding gate (new):** pressing **Start** now checks for Accessibility
  permission first. If it is missing, Deskflow shows a dialog that triggers the
  native macOS permission prompt and opens
  **System Settings → Privacy & Security → Accessibility**, instead of silently
  launching a core that fails. See `src/lib/gui/MainWindow.cpp` (`ensureAccessibilityPermission`).
- **HID passthrough (unchanged):** macOS key injection still goes through
  `OSXKeyState::postHIDVirtualKey()` → `IOHIDPostEvent()`, with a CGEvent fallback
  (`src/lib/platform/OSXKeyState.cpp`). The onboarding change does not touch this path.

> Note: this fork uses Deskflow's own input system. It does **not** depend on
> Karabiner-Elements, and there is no account/login. Those belong to the separate
> commercial *Synergy* product, not this project.

---

## Step 0 — Keep the fork up to date with upstream

Run this once per clone (these commands must be run by you, from a machine with
network access to GitHub — CI/cloud agents are scoped to the fork only):

```bash
git remote add upstream https://github.com/deskflow/deskflow.git   # first time only
git fetch upstream
git checkout master && git merge --ff-only upstream/master && git push origin master
# rebase this feature branch onto the refreshed master:
git checkout claude/karibeener-extension-install-mggwmq && git rebase master
git push --force-with-lease origin claude/karibeener-extension-install-mggwmq
```

## Step 1 — Get the code on each device

```bash
git clone https://github.com/hughesyadaddy/deskflow.git
cd deskflow
git checkout claude/karibeener-extension-install-mggwmq
```

## Step 2 — Build & install

Requirements (all platforms): CMake 3.24+, Qt 6.7+, OpenSSL 3.0+.
(`google_test` is fetched automatically if missing.)

### macOS (including a macOS VM on Proxmox)

```bash
# Toolchain + deps via Homebrew
brew install cmake qt openssl@3

# Configure, build, and produce the .app bundle
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# The bundle lands at build/bin/Deskflow.app — copy it to /Applications
cp -R build/bin/Deskflow.app /Applications/
xattr -c /Applications/Deskflow.app    # clear quarantine on unsigned local builds
```

Then launch Deskflow and press **Start**. When prompted, grant **Accessibility**
to **both** `Deskflow` and the `deskflow` process under
System Settings → Privacy & Security → Accessibility. On macOS Sequoia, also allow
**Local Network**.

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
```

See `docs/dev/build.md` for the full Windows/vcpkg details.

## Step 3 — Optional: build installable packages

```bash
cmake --build build --target package        # dmg (macOS), deb/rpm/archive (Linux), etc.
```

---

## If it crashes — collect this and send it

The onboarding/HID code can't be fixed blind. On a machine that crashes, grab the
stack trace:

```bash
# macOS crash report (preferred — has the faulting line):
cat "$(ls -t ~/Library/Logs/DiagnosticReports/Deskflow* 2>/dev/null | head -1)"

# Deskflow's own log (set log level to DEBUG in Settings first):
tail -200 ~/Library/Logs/Deskflow/*.log
```

Paste the output back so the crash can be pinpointed and patched on this branch.
