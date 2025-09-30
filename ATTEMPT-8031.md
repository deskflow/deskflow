## Attempt — Implementation plan for #8031

**Scope (required by bounty):** Make clipboard sharing work on Wayland on **GNOME (Mutter)** and **KDE (KWin)** via **XDG portals**; land needed changes in portal backends/compositors and integrate Deskflow. No extra "enterprise" features.

**Flow (now): RemoteDesktop + Clipboard**
- Create RemoteDesktop session; **before** Start(), call org.freedesktop.portal.Clipboard.RequestClipboard(session) (spec requires *before* start).
- Implement: SetSelection (advertise mimetypes), handle SelectionOwnerChanged / SelectionTransfer, implement SelectionRead, SelectionWrite, SelectionWriteDone.
- Files later via FileTransfer (pplication/vnd.portal.filetransfer) after parity on other platforms.

**Backends:**
- xdg-desktop-portal-gnome and xdg-desktop-portal-kde implement/verify Clipboard portal, using **ext-data-control-v1** to talk to the compositor.

**Compositors:**
- Ensure **KWin** and **Mutter** expose required data-control hooks used by the portal backends.

**Deskflow integration:**
- Use **QDBus** to call org.freedesktop.portal.Clipboard.* on org.freedesktop.portal.Desktop at /org/freedesktop/portal/desktop. Keep libportal for other portals if desired.

**Acceptance checks (both GNOME & KDE):**
- Bidirectional copy/paste of 	ext/plain, 	ext/html, image/png
- Large payloads; rapid ownership churn; demo video included
- Troubleshooting notes for portal/backend selection

**Deliverables:**
- Upstream MRs: GNOME portal backend, KDE portal backend, KWin/Mutter (if needed)
- Single Deskflow PR linking all MRs; CI green; demo video and repro steps

*(Spec reference for reviewers: Clipboard portal requires RequestClipboard **before** session starts; RemoteDesktop describes enabling clipboard via this request.)*
