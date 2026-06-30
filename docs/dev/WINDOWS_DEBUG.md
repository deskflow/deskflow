# Windows debug workflow (Deskflow GUI + deskflow-core)

Deskflow on Windows ships separate executables in the build output directory:

| Binary | Debug path | Role |
|--------|------------|------|
| **deskflow** (GUI) | `build-debug/bin/Debug/deskflow.exe` | Settings, tray, Start/Stop |
| **deskflow-core** | `build-debug/bin/Debug/deskflow-core.exe` | Server / client / auto KVM engine |
| **deskflow-daemon** | `build-debug/bin/Debug/deskflow-daemon.exe` | Service helper (installed app) |

Production behavior: launch GUI → press **Start** → GUI spawns `deskflow-core` from the
same directory via `QProcess`. The VS Code launcher mirrors that; it does **not** auto-start
core on GUI launch.

## Build directories

| Directory | Build type | Used for |
|-----------|------------|----------|
| `build/` | Release | `scripts/install-windows.ps1` → Program Files |
| `build-debug/` | Debug | F5, Run tasks, local iteration |

Debug builds never overwrite Program Files unless you explicitly run the install script.

## Settings file

Debug and Release both use `%APPDATA%\Deskflow\Deskflow.conf` (same as the installed app).
Server layout: `%APPDATA%\Deskflow\deskflow-server.conf`. TLS: `%APPDATA%\Deskflow\tls\`.

**Prepare Debug Session (Windows)** runs `scripts/sync-debug-settings-windows.ps1`, which
fills in `coordination/peers` from your server screen names when Auto mode would otherwise
block Start.

### Sharing (fork extension)

Logitech / Mouser sharing settings live in `Deskflow.conf`, not the server layout file:

| Setting | Where to configure |
|---------|-------------------|
| `server/hidPassthrough*` / `server/mouserBridge*` | **Server Configuration → Sharing** (server or auto mode) |
| `client/mouserEnabled` / `client/mouserToken` | **Client Configuration → Sharing** (client mode) or mirrored when saving server sharing |

Configure sharing in the GUI on each machine before debugging gesture relay. See
`docs/mouser-bridge.md` and `docs/hid-passthrough.md`.

## VS Code / Cursor tasks

| Task | What it does |
|------|----------------|
| **Build, Sign & Install Deskflow** (default) | Release build → Program Files |
| **Deskflow: Prepare Debug Session (Windows)** | Quit all → sync production settings → configure + build `build-debug/` |
| **Deskflow: Sync production settings (Windows)** | Derive `coordination/peers` from server layout if missing |
| **Deskflow: Run GUI (Debug, local)** | Build and launch `deskflow.exe --show` from debug output |
| **Deskflow: Run Core (Debug, client/server/auto)** | Run `deskflow-core` directly from debug output |
| **Deskflow: Quit all (Windows)** | Stop service, GUI, core, daemon, vhid-bridge |

## VS Code / Cursor launch configs (F5)

| Config | When to use |
|--------|-------------|
| **Deskflow GUI (Windows Debug)** | GUI PRs — press **Start** in the app to spawn core |
| **deskflow-core (Debug, client/server/auto)** | Core-only PRs — uses `--settings %APPDATA%\Deskflow\Deskflow.conf` |
| **Attach to deskflow-core (Windows)** | Full-stack: debug GUI, press Start, then attach to core PID |

Requires the **C/C++** extension (`ms-vscode.cpptools`) with `cppvsdbg` on Windows.

### GUI → core under the debugger

F5 on the GUI config only debugs the GUI process. Core breakpoints require either:

1. **Attach** after pressing Start (pick `deskflow-core.exe` in the process list), or
2. A **core-only** launch config for the code you are changing.

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| "already running" / core won't start | Run **Quit all (Windows)**; stop the installed service/GUI |
| Breakpoints in core don't hit (GUI F5) | Use core launch config or Attach after Start |
| Install task killed my debug session | Default install stops all Deskflow processes — expected |
| Start disabled in Auto mode | Run **Sync production settings** or add peers in the auto-switch list |
| Gestures dead on client | Open **Client Configuration → Sharing**; enable Mouser + matching token |
| Gestures dead on server | Open **Server Configuration → Sharing**; enable HID passthrough or bridge |

## Copying launcher config to a new machine

`.vscode/` is gitignored in this fork. Copy from `docs/dev/launch.json.example` and
`docs/dev/tasks.json.example` into `.vscode/`.
