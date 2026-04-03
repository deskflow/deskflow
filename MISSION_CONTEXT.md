# Mission Context: XDG Desktop Portal Clipboard Support

**Issue:** [#8031](https://github.com/deskflow/deskflow/issues/8031)
**Branch:** `autogamer/xdg-clipboard`
**Status:** Ready for PR submission

## Commits

1. `d4b9600` — `feat(platform): Add XDG Portal clipboard backend for Wayland`
2. `7b40350` — `feat(platform): Add PortalClipboard implementation and tests`

## What was implemented

- `PortalClipboard` class using libportal API (`xdp_portal_access_clipboard`, `selection-owner-changed` signal)
- `WlClipboardCollection` backend selection: portal (preferred) → wl-clipboard (fallback)
- 25 unit tests following WlClipboardTests patterns
- Documentation: `docs/dev/portal-clipboard.md`, `docs/dev/build.md` Wayland section

## Key design decisions

- Portal path disabled (`#if 0`) in WlClipboardCollection until xdg-desktop-portal backends support clipboard
- Conditional compilation with `XDP_CHECK_VERSION(0, 9, 1)`
- Graceful fallback: no regression in existing wl-clipboard functionality
- Follows existing WlClipboard patterns (same base classes, method signatures, caching)

## Files changed (10 files, 1931 insertions)

| File | Change |
|------|--------|
| `src/lib/platform/PortalClipboard.h` | New — PortalClipboard header |
| `src/lib/platform/PortalClipboard.cpp` | New — PortalClipboard implementation |
| `src/lib/platform/WlClipboardCollection.h` | Modified — Backend enum + methods |
| `src/lib/platform/WlClipboardCollection.cpp` | Modified — Backend selection logic |
| `src/lib/platform/CMakeLists.txt` | Modified — Add PortalClipboard sources |
| `src/unittests/platform/PortalClipboardTests.h` | New — Test class header |
| `src/unittests/platform/PortalClipboardTests.cpp` | New — 25 test cases |
| `src/unittests/platform/CMakeLists.txt` | Modified — Add test target |
| `docs/dev/portal-clipboard.md` | New — Portal clipboard documentation |
| `docs/dev/build.md` | Modified — Wayland clipboard section |

## Reviewer notes

- `#if 0` in WlClipboardCollection.cpp is intentional (awaiting upstream portal support)
- Build not verified locally due to missing system deps (qt6-tools, libportal) — CI should verify
- No runtime testing possible (portal clipboard not yet available in any distro)
- PR description in `PR_DESCRIPTION.md`
