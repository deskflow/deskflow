# macOS debug workflow (Deskflow GUI + deskflow-core)

Deskflow on macOS ships as a single bundle with two binaries:

| Binary | Path inside bundle | Role |
|--------|-------------------|------|
| **Deskflow** (GUI) | `Deskflow.app/Contents/MacOS/Deskflow` | Settings, tray, Start/Stop |
| **deskflow-core** | `Deskflow.app/Contents/MacOS/deskflow-core` | Server / client / auto KVM engine |

Production behavior: launch GUI → press **Start** → GUI spawns `deskflow-core` from the
same bundle via `QProcess` (`CoreProcess.cpp`). The VS Code launcher mirrors that; it does
**not** auto-start core on GUI launch.

## Build directories

| Directory | Build type | Used for |
|-----------|------------|----------|
| `build/` | Release | `scripts/install-macos.sh` → `/Applications/Deskflow.app` |
| `build-debug/` | Debug | F5, Run tasks, local iteration |

Debug builds never overwrite `/Applications` unless you explicitly point the install script
at a different path.

## Before you debug

1. **Quit the installed app** if it is running (`/Applications/Deskflow.app`). GUI and core
   use single-instance locks; a running install blocks debug builds.
2. **Accessibility (TCC)** is granted **per binary path**. Enable both:
   - `build-debug/bin/Deskflow.app` → Deskflow
   - `build-debug/bin/Deskflow.app` → deskflow-core (may appear as `deskflow` or `deskflow-core`)
3. Settings file is shared: `~/Library/Deskflow/Deskflow.conf` (same for debug and Release).
   Server layout: `~/Library/Deskflow/deskflow-server.conf`. TLS: `~/Library/Deskflow/tls/`.
   **Prepare Debug Session** runs `scripts/sync-debug-settings-macos.sh`, which fills in
   `coordination/peers` from your server screen names when Auto mode would otherwise block Start.

Sharing (HID passthrough / Mouser) is configured in **Server Configuration → Sharing**
(server/auto mode) or **Client Configuration → Sharing** (client mode). Settings are stored
in `Deskflow.conf`, not the server layout file. See `docs/mouser-bridge.md`.

## VS Code / Cursor tasks

| Task | What it does |
|------|----------------|
| **Build, Sign & Install Deskflow** (default) | Release build → `/Applications` |
| **Deskflow: Prepare Debug Session (macOS)** | Quit all → sync production settings → configure + build `build-debug/` |
| **Deskflow: Sync production settings (macOS)** | Derive `coordination/peers` from server layout if missing |
| **Deskflow: Run GUI (Debug, local)** | Build and `open build-debug/.../Deskflow.app --show` |
| **Deskflow: Run Core (Debug, client/server/auto)** | Run core directly from debug bundle |
| **Deskflow: Quit all (macOS)** | Stop GUI, core, vhid-bridge |

## VS Code / Cursor launch configs (F5)

| Config | When to use |
|--------|-------------|
| **Deskflow GUI (macOS Debug)** | GUI PRs — press **Start** in the app to spawn core |
| **deskflow-core (Debug, client/server/auto)** | Core-only PRs — uses `--settings ~/Library/Deskflow/Deskflow.conf` |
| **Attach to deskflow-core (macOS)** | Full-stack: debug GUI, press Start, then attach to core PID |

### GUI → core under the debugger

F5 on the GUI config only debugs the GUI process. Core breakpoints require either:

1. **Attach** after pressing Start (pick `deskflow-core` in the process list), or
2. A **core-only** launch config for the code you are changing.

This matches how production works: separate processes, GUI orchestrates core.

## PR strategy (keeping GUI and core separable)

Upstream `deskflow/deskflow` treats GUI and core as separate CMake targets even though
macOS bundles them together. Typical split:

- **GUI PR** — settings UI, tray, dialogs → debug with **Deskflow GUI** config
- **Core PR** — KVM, bridge, input → debug with **deskflow-core** config
- **Integration PR** — GUI spawn + IPC → GUI debug + **Attach to deskflow-core**

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| "already running" / core won't start | Run **Quit all (macOS)**; quit `/Applications/Deskflow.app` |
| Core start fails immediately | Check Accessibility for debug binary paths |
| Breakpoints in core don't hit (GUI F5) | Use core launch config or Attach after Start |
| Install task killed my debug session | Default install runs **Quit all** patterns — expected |
| Start disabled in Auto mode | Run **Sync production settings** or add peers in the auto-switch list |
| Stale binary | **Prepare Debug Session** rebuilds `build-debug/` |

## Copying launcher config to a new machine

`.vscode/` is gitignored in this fork. Copy from `docs/dev/launch.json.example` and
`docs/dev/tasks.json.example` into `.vscode/`, or sync your local `.vscode/` folder manually.
