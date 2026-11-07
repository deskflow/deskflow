# Wayland notes

See also the [known Wayland bugs discussion](https://github.com/deskflow/deskflow/discussions/7499).

## Client on Hyprland / wlroots-style compositors

Hyprland's portal does not yet provide a full RemoteDesktop `ConnectToEIS` path for clients.
A common workaround is to route RemoteDesktop (and Clipboard) to [xdg-desktop-portal-luminous](https://github.com/waycrate/xdg-desktop-portal-luminous):

```ini
# ~/.config/xdg-desktop-portal/hyprland-portals.conf
[preferred]
default=hyprland;gtk
org.freedesktop.impl.portal.RemoteDesktop=luminous
org.freedesktop.impl.portal.ScreenCast=luminous
org.freedesktop.impl.portal.Clipboard=luminous
```

Restart the portal stack after changing this (`systemctl --user restart xdg-desktop-portal.service`).

### Fractional scale (cursor bounce)

If the client monitor uses fractional scale (for example 1920×1080 at 1.25 → logical 1536×864), the portal must advertise **logical** EIS region sizes.
When the portal reports physical pixels instead, Deskflow sends a physical shape (e.g. `1920x1080`) and absolute pointer motion hits the logical edge → rapid enter/leave and a cursor that will not stay on the client.

Check the client log for `logical output size` / `sending info shape=`. The shape should match the compositor's logical size, not the physical mode.

Related luminous fix: [waycrate/xdg-desktop-portal-luminous#244](https://github.com/waycrate/xdg-desktop-portal-luminous/pull/244).

### Clipboard restore token discard

If the RemoteDesktop session starts without clipboard enabled, Deskflow may discard the restore token once and reconnect.
Routing `Clipboard=luminous` (as above) lets luminous report `clipboard_enabled` so the token is kept.

### Logging and mouse lag

`VERBOSE` (or expanded GUI log view) while moving the mouse writes a huge amount of EIS traffic and can peg the GUI process.
Use `INFO` for normal use; switch to `DEBUG`/`VERBOSE` only while capturing a short repro, then turn it back down.

## Server on Hyprland

Input Capture for server mode also needs a portal backend that implements it.
See the portal lists in the [known Wayland bugs discussion](https://github.com/deskflow/deskflow/discussions/7499).
