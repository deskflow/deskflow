# Wayland clipboard compatibility mode

Deskflow normally uses the desktop portal for clipboard sharing. Some builds,
including builds against libportal 0.9.1, do not have the portal clipboard API.
Their keyboard and mouse connection can work while clipboard sharing does not.

An opt-in compatibility backend restores the `wl-copy` / `wl-paste` path used
before 1.27. Start Deskflow with:

```sh
DESKFLOW_WL_CLIPBOARD_FALLBACK=1 deskflow
```

This option applies only to builds without the portal clipboard API. Builds
with `HAVE_LIBPORTAL_CLIPBOARD` continue to use their portal backend.

Both commands must be available in `PATH` and able to access the local clipboard.
The variable is inherited by `deskflow-core`. Leaving it unset keeps the portal
backend unchanged. A log message identifies when the compatibility backend is in
use. The existing clipboard-sharing switch and size limit still apply.

This does not add a missing compositor clipboard protocol. Stock `wl-clipboard`
tools may not work on GNOME; systems using an existing, scoped XWayland adapter
can keep that adapter in Deskflow's `PATH`. The compatibility backend restores
the previous backend's limitations, including the lack of rich HTML writes.
Use the portal backend when it is available and works with your compositor.
