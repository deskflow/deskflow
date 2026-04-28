# Deskflow 1.26.0 — Portal Clipboard (Self-Built Release)

**Commit:** `36007f89fb930a153e85a167af8f90a2d51cc9da`
**Branch:** `feature/portal-clipboard`
**Date:** 2026-04-28 09:49:12 +0800
**Version:** 1.26.0.9999

## What's Included

| Binary | Description |
|--------|-------------|
| `deskflow` | Qt GUI application (24 MB) |
| `deskflow-core` | Core daemon — server + client (33 MB) |
| `deskflow-core-wayland` | Wrapper — launches deskflow-core with `XDG_SESSION_TYPE=wayland` |
| `deskflow-gui-wayland` | Wrapper — launches deskflow GUI with Wayland + Qt platform plugin |

## Changes vs Upstream

### Portal Clipboard Support
- XDG Desktop Portal clipboard via `org.freedesktop.portal.Clipboard` (GNOME portal)
- Automatic fallback to wl-clipboard if portal is unavailable
- No focus-grabbing on GNOME/Wayland (avoids wl-copy/wl-paste focus steal)

### Bug Fixes
1. **Client-side clipboard AccessDenied** — Client now creates a separate `clipboardOnly=true` session on both primary and non-primary sides, since libportal 0.9.1 doesn't expose `XDP_REMOTE_DESKTOP_CAPABILITY_CLIPBOARD`.
2. **SelectionRead fd EAGAIN** — Portal clipboard fd is non-blocking (O_NONBLOCK); now set to blocking mode before reading.
3. **GNOME portal preferred over GTK portal** — Session handle discovery prefers GNOME portal to avoid GTK portal limitations.

## Installation

### Option 1: Quick Install (User-Level)
```bash
cp bin/* ~/.local/bin/
# Then use deskflow-gui-wayland or deskflow-core-wayland to launch
```

### Option 2: System-Wide
```bash
sudo cp bin/* /usr/local/bin/
```

## Usage

### Server (Primary)
```bash
deskflow-core-wayland --server --address :24800
```

### Client (Non-Primary)
```bash
deskflow-core-wayland --client <server-hostname>:24800
```

### GUI
```bash
deskflow-gui-wayland
# or
deskflow-wayland   # if you renamed the wrapper
```

## Dependencies
- libportal >= 0.9.1
- xdg-desktop-portal (GNOME)
- Qt 6 (for GUI)
- libei / libportal-input-capture

## Known Issues
- "Missing Keyboard Layouts" dialog may appear on systems without Chinese keyboard layout installed. Fix: Settings → Keyboard → Input Sources → Add Chinese.
- GUI may fail to connect if launched without the Wayland wrapper (ensure `XDG_SESSION_TYPE=wayland` is set).
