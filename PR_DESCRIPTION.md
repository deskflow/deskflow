## feat(platform): Add XDG Desktop Portal clipboard backend for Wayland

Addresses #8031 (client-side implementation)

### Summary

Implements `PortalClipboard`, an `IClipboard` backend using the XDG Desktop Portal D-Bus API (`org.freedesktop.portal.Clipboard`) via libportal. This enables clipboard access in sandboxed environments (Flatpak, Snap) on Wayland.

The client-side implementation is **complete and ready for merge**, but awaits xdg-desktop-portal backends (Mutter, KWin) adding clipboard portal support before it becomes functionally active. Until then, Deskflow continues using the existing wl-clipboard backend.

### Key changes

- **`src/lib/deskflow/IClipboard.h`** — Added 5 virtual methods for clipboard monitoring: `hasChanged()`, `startMonitoring()`, `stopMonitoring()`, `resetChanged()`, `getID()`. Stub implementations added to Windows/macOS/X11 clipboard classes.
- **`src/lib/platform/PortalClipboard.h/cpp`** — New `PortalClipboard` class inheriting `QObject` + `IClipboard`, using `xdp_portal_access_clipboard()` and `selection-owner-changed` signal. Thread-safe with mutex-protected cache. Conditionally compiled when libportal >= 0.9.1.
- **`src/lib/platform/WlClipboardCollection.h/cpp`** — Added `ClipboardBackend` enum and backend selection logic. **Polymorphic storage**: `m_clipboards` now holds `unique_ptr<IClipboard>` to support both `WlClipboard` and `PortalClipboard`. Portal path currently disabled (`#if 0`) pending upstream support.
- **`src/lib/platform/CMakeLists.txt`** — Adds PortalClipboard sources when `LIBPORTAL_FOUND`.
- **`src/unittests/platform/PortalClipboardTests.h/cpp`** — 25 test cases covering all public methods, following existing WlClipboardTests patterns. Conditionally compiled with `WINAPI_LIBPORTAL`.
- **`docs/dev/portal-clipboard.md`** — Comprehensive documentation of D-Bus interface, architecture, configuration, and testing instructions.
- **`docs/dev/build.md`** — Wayland clipboard section documenting portal and wl-clipboard backends.

### Architecture

```
WlClipboardCollection
├── Backend Selection
│   ├── Portal (preferred) → PortalClipboard (libportal 0.9.1+)
│   └── wl-clipboard (fallback) → WlClipboard (wl-copy/wl-paste)
└── Polymorphic Storage (unique_ptr<IClipboard>)
    ├── kClipboardClipboard (standard)
    └── kClipboardSelection (primary)

IClipboard Interface (new monitoring methods)
├── hasChanged(), startMonitoring(), stopMonitoring()
├── resetChanged(), getID()
└── Implementations: PortalClipboard, WlClipboard, MSWindowsClipboard, OSXClipboard, XWindowsClipboard
```

### Test plan

- [ ] CI build passes with libportal >= 0.9.1
- [ ] CI build passes without libportal (portal sources excluded)
- [ ] Unit tests pass: `ctest --test-dir build -R PortalClipboard --output-on-failure`
- [ ] Existing wl-clipboard functionality unchanged (fallback path)
- [ ] No regression in X11 clipboard support
- [ ] Code review of thread safety (mutex/atomic usage)
- [ ] Verify `#if 0` disable block in WlClipboardCollection is intentional

### Notes for reviewers

- The `#if 0` block in `WlClipboardCollection.cpp` is intentional — it disables portal backend selection until xdg-desktop-portal backends support clipboard. See inline comments.
- The `#else` branches in `PortalClipboard.cpp` handle older libportal versions (< 0.9.1) that lack the clipboard access API.
- `LOG_DEBUG` calls throughout PortalClipboard use the project's standard logging macro, not debug prints.

### Known limitations

- **No runtime verification**: Portal clipboard requires xdg-desktop-portal backends (Mutter, KWin) to implement clipboard support, which is not yet available in any distribution.
