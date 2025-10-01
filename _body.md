/claim #8031

﻿## Attempt — Implementation plan for #8031

**Scope (per bounty):** Make clipboard sharing work on Wayland on **GNOME (Mutter)** and **KDE (KWin)** via **XDG portals**; land needed changes in portal backends/compositors and integrate Deskflow. No extra "enterprise" features.

**Flow (now): RemoteDesktop + Clipboard**
- Create RemoteDesktop session; **before** Start(), call org.freedesktop.portal.Clipboard.RequestClipboard(session) (spec requires *before* start).  
- Implement: SetSelection (advertise MIME types), handle SelectionOwnerChanged / SelectionTransfer, implement SelectionRead, SelectionWrite, SelectionWriteDone.  
- Files later via FileTransfer (^Gpplication/vnd.portal.filetransfer) after parity on other platforms.

**Backends:**
- xdg-desktop-portal-gnome and xdg-desktop-portal-kde implement/verify Clipboard portal, talking to the compositor via **ext-data-control-v1**.

**Compositors:**
- Ensure **KWin** and **Mutter** expose the required data‑control hooks used by the portal backends (ext‑data‑control‑v1).

**Deskflow integration:**
- Use **QDBus** to call org.freedesktop.portal.Clipboard.* on org.freedesktop.portal.Desktop at /org/freedesktop/portal/desktop. Keep libportal for other portals if desired.

**Acceptance checks (both GNOME & KDE):**
- Bidirectional copy/paste of 	text/plain, 	text/html, image/png
- Large payloads; rapid ownership churn; short demo video included
- Troubleshooting notes for portal/backend selection

**Deliverables:**
- Upstream MRs: GNOME portal backend, KDE portal backend, KWin/Mutter (if needed)
- Single Deskflow PR linking all MRs; CI green; demo video and repro steps

*Spec references for reviewers:*  
• **Clipboard.RequestClipboard** “This request must be made **before** the session starts … see Remote Desktop to integrate clipboard.” :contentReference[oaicite:1]{index=1}  
• **RemoteDesktop.Start** and session flow (Clipboard enabled via RequestClipboard). :contentReference[oaicite:2]{index=2}  
• **ext‑data‑control‑v1** gives the portal backend privileged clipboard control. :contentReference[oaicite:3]{index=3}
