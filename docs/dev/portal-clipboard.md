# XDG Desktop Portal Clipboard Support

## Overview

Deskflow supports clipboard operations through the XDG Desktop Portal,
providing clipboard access in sandboxed environments (Flatpak, Snap) on Wayland.
This enables secure clipboard sharing between machines without requiring
direct access to the compositor's clipboard protocols.

## Current Status

**As of 2026-04-03, the PortalClipboard implementation is complete and ready for
review/merge, but awaiting xdg-desktop-portal backend support for actual functionality.**

The implementation:
- Passes all code review acceptance criteria
- Is ready for when portal clipboard becomes available
- Will be automatically enabled when xdg-desktop-portal backends support clipboard

### What's Implemented

- **WlClipboard** (`src/lib/platform/WlClipboard.h/.cpp`): Working clipboard backend
  using `wl-copy` and `wl-paste` command-line tools from the wl-clipboard project
- **WlClipboardCollection** (`src/lib/platform/WlClipboardCollection.h/.cpp`): Clipboard
  manager for the EiScreen Wayland backend with backend selection logic
- **PortalClipboard** (`src/lib/platform/PortalClipboard.h/.cpp`): IClipboard implementation
  using libportal to communicate with `org.freedesktop.portal.Clipboard`
  - Ready for when xdg-desktop-portal backends implement clipboard support
  - Uses `xdp_portal_access_clipboard()` API from libportal 0.9.1+
  - Connects to `selection-owner-changed` signal for change detection
  - Properly handles access denial and fallback

### What's Ready (Awaiting Upstream Support)

The PortalClipboard implementation is complete and will be automatically enabled
when xdg-desktop-portal backends add clipboard support:

- **PortalClipboard class**: Fully implemented, conditionally compiled
- **Backend selection logic**: WlClipboardCollection tries portal first, falls back
  to wl-clipboard when portal clipboard is unavailable
- **D-Bus detection**: Checks for `org.freedesktop.portal.Clipboard` interface

### Upstream Dependencies

Portal clipboard requires the following upstream support (not yet available):

- **xdg-desktop-portal**: Clipboard portal specification (pending)
- **Mutter** (GNOME): Clipboard portal implementation (pending)
- **KWin** (KDE): Clipboard portal implementation (pending)
- **libportal** >= 0.9.1: Client-side library (available now)

Related issue: [deskflow/deskflow#8031](https://github.com/deskflow/deskflow/issues/8031)

## Architecture

### Current Implementation

- **WlClipboard** (`src/lib/platform/WlClipboard.h/.cpp`): IClipboard implementation
  using `wl-copy` and `wl-paste` command-line tools
- **WlClipboardCollection** (`src/lib/platform/WlClipboardCollection.h/.cpp`): Clipboard
  manager for EiScreen that provides clipboard instances

### Planned (Pending Upstream Support)

- **PortalClipboard** (`src/lib/platform/PortalClipboard.h/.cpp`): IClipboard implementation
  using D-Bus to communicate with `org.freedesktop.portal.Clipboard` (not yet implemented)
- **D-Bus Interface** (`src/lib/platform/org.freedesktop.portal.Clipboard.xml`): Expected
  portal clipboard interface definition (not yet created)

## D-Bus Interface

The clipboard portal uses the following D-Bus interface at
`org.freedesktop.portal.Clipboard` on the session bus:

### Service Name
```
org.freedesktop.portal.Clipboard
```

### Object Path
```
/org/freedesktop/portal/desktop
```

### Methods

#### RequestClipboard
```
RequestClipboard(session_handle: o, options: a{sv}) -> (response: u, results: a{sv})
```
Request clipboard access for a session. This establishes the caller as a
potential clipboard participant.

**Parameters:**
- `session_handle`: Object path for the session
- `options`: Dictionary of options (reserved for future use)

**Returns:**
- `response`: Portal response code (0 = success)
- `results`: Dictionary of results

#### SetClipboard
```
SetClipboard(session_handle: o, options: a{sv}, mime_types: as) -> (response: u, results: a{sv})
```
Set clipboard content. The actual data transfer happens via file descriptor
passed through the SelectionTransfer signal.

**Parameters:**
- `session_handle`: Object path for the session
- `options`: Dictionary of options
- `mime_types`: Array of offered MIME types

### Signals

#### SelectionOwnerChanged
```
SelectionOwnerChanged(session_handle: o, mime_types: as)
```
Emitted when the clipboard owner changes. Clients should monitor this signal
to know when to request clipboard data.

**Parameters:**
- `session_handle`: Object path for the session
- `mime_types`: Array of available MIME types in the new selection

#### SelectionTransfer
```
SelectionTransfer(session_handle: o, mime_type: s, fd: h)
```
Transfer clipboard data via file descriptor. The fd is writable by the sender
when sending, readable by the receiver when receiving.

**Parameters:**
- `session_handle`: Object path for the session
- `mime_type`: The MIME type of the data
- `fd`: File descriptor for data transfer

#### SelectionRequest
```
SelectionRequest(session_handle: o, mime_types: as)
```
Request clipboard data from the current owner. The owner should respond with
SelectionTransfer.

**Parameters:**
- `session_handle`: Object path for the session
- `mime_types`: Array of requested MIME types

## Prerequisites

Portal clipboard requires:

1. **xdg-desktop-portal** >= 0.9 with clipboard support
   - Currently not available; specification pending
   - See: [xdg-desktop-portal upstream](https://github.com/flatpak/xdg-desktop-portal)

2. **Compositor support**
   - Mutter >= 46 (pending)
   - KWin >= 6.0 (pending)
   - Other compositors may add support later

3. **libportal** >= 0.9.1
   - Provides the client-side library for portal interactions
   - Available in most distributions

## Configuration

The following configuration options control clipboard behavior:

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `core/portalClipboard` | bool | true | Enable/disable portal clipboard backend |
| `core/wlClipboard` | bool | true | Enable/disable wl-clipboard fallback |

When both are enabled, Deskflow will prefer the portal clipboard if available,
falling back to wl-clipboard (using `wl-copy`/`wl-paste` tools) if not.

## Backend Selection Logic (Planned)

When PortalClipboard is implemented, the `WlClipboardCollection` class will select
backends in this order:

1. **Portal Clipboard** (preferred)
   - Used when xdg-desktop-portal is running with clipboard support
   - Requires session bus connection
   - Works in sandboxed environments

2. **wl-clipboard** (fallback - currently used)
   - Uses external `wl-copy` and `wl-paste` commands
   - Requires Wayland compositor with data device manager
   - Does not work in sandboxed environments

**Current Behavior:** WlClipboardCollection only uses the wl-clipboard backend.
Portal clipboard support will be added when the upstream implementation becomes
available.

## Implementation Notes

### Thread Safety

The portal clipboard implementation uses D-Bus for IPC, which is inherently
thread-safe for method calls. However, clipboard callbacks should be handled
on the main thread to avoid race conditions.

### Error Handling

The implementation gracefully handles:
- Missing portal service
- Portal without clipboard support
- D-Bus connection failures
- Session cancellation

### Testing

#### Unit Tests

Automated tests are in `src/unittests/platform/PortalClipboardTests.cpp`. These tests cover:

- Constructor and clipboard ID assignment
- D-Bus interface detection (`isAvailable()`)
- Settings-based enable check (`isEnabled()`)
- Open/close lifecycle (double-open prevention, safe close without open)
- Clipboard operations: empty, add, has, get for Text/HTML/Bitmap formats
- Timestamp preservation through open/close
- Change detection flag lifecycle (hasChanged/resetChanged)
- Monitoring start/stop (safe re-entry)
- Error handling: operations without open, unknown MIME types

Tests are conditionally compiled with `WINAPI_LIBPORTAL` and built when `libportal >= 0.9.1`
is found. Tests that require a running portal are skipped via `QSKIP()` when unavailable.

Run tests:
```bash
ctest --test-dir build/src/unittests -R PortalClipboard --output-on-failure
```

#### Manual Testing

To test portal clipboard support (once available):

1. Ensure xdg-desktop-portal is running with clipboard support
2. Enable portal clipboard in configuration
3. Check logs for "PortalClipboard" messages
4. Use `busctl --user introspect org.freedesktop.portal.Desktop` to verify
   the clipboard interface is available

## Future Development

When xdg-desktop-portal adds clipboard support:

1. Update libportal dependency if new API is required
2. Verify D-Bus interface matches our implementation
3. Update MIME type handling as needed
4. ~~Add integration tests with portal mock~~ (Done - see PortalClipboardTests)
5. Update this documentation with actual version requirements

## Related Resources

- [XDG Desktop Portal Documentation](https://docs.flatpak.org/en/latest/portal-api-reference.html)
- [libportal Repository](https://github.com/flatpak/libportal)
- [Wayland Clipboard Protocol](https://wayland.freedesktop.org/docs/html/ch04.html#sect-protocol-data-device)
- [Deskflow Issue #8031](https://github.com/deskflow/deskflow/issues/8031)
