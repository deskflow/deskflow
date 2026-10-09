# Send files and windows to another Mac

Install this build as `/Applications/Deskflow.app` on each Mac. Both computers
must be logged into a desktop session. Sending uses macOS SSH separately from
Deskflow's keyboard and mouse connection; the receiving GUI need not be running.

On the receiving Mac, enable **System Settings > General > Sharing > Remote
Login** for your account. Set up SSH key login, then connect once from the sending
Mac with `ssh user@computer.local`. Check the host fingerprint before accepting it.
Deskflow requires a trusted host key and non-interactive key login; it never
accepts a new host key automatically. SSH aliases in `~/.ssh/config` work too,
including custom ports. Deskflow does not change Remote Login or SSH configuration.

In Finder, select files, right-click, and choose **Services > Send to computer…**.
Choose a computer from your Deskflow layout or enter `user@computer.local` (or an
SSH alias). Successful destinations are remembered. If the service is hidden,
enable it in **System Settings > Keyboard > Keyboard Shortcuts > Services**.
Restart Finder or log out and back in if macOS has not registered the new service.

For an application, right-click its `.app` in Finder and use the same service.
If it is running, Deskflow sends its current saved document or browser URL. If
it is not running, it opens the installed app on the destination. From any app,
you can also use **Send current window to computer…** in Deskflow's menu-bar icon;
this uses the most recently active app other than Deskflow. The Deskflow File
menu also has file and current-window sending actions.

Safari and Google Chrome use Apple Events to read the active tab. Allow Deskflow
under **Privacy & Security > Automation** when prompted. Other apps need to expose
their current saved document or URL through Accessibility. If an app does not
expose it, save the document and send the file from Finder instead. The same app
must be installed on the other Mac, with the same bundle identifier.

Files are copied into a fresh `Downloads/deskflow-…` folder, never over an existing
file. Incomplete transfers are discarded when the receiver exits normally. A hard
crash can leave a hidden `.deskflow-…` staging folder in Downloads. Plain file sending does not open or run the
files. App handoff opens the copy with the specified installed app. A failure to
open the app leaves the completed copy in Downloads. Cancel stops an unfinished
transfer; if it has already arrived, cancel cannot undo delivery or app opening.

## Current limits

- Mac-to-Mac only. This does not stream a window or move a running process.
- Save before sending. Unsaved edits, browser logins, app state, and subsequent
  changes stay on the original computer. Sending a copy does not create a sync.
- Regular files only, at most 1024 per transfer and 1 TiB per file. Folders,
  symbolic links, and package documents are rejected. Files with identical names
  (including case and Unicode equivalents) must be sent separately.
- Copies contain file data; resource forks, extended attributes, tags, ACLs and
  executable permissions are not transferred. Do not use this to copy applications.
- Browser handoff accepts HTTP/HTTPS URLs without embedded credentials. Internal
  browser pages and other URL schemes are rejected.
- Finder's service adds a Send action with a computer picker. It does not inject
  a menu into the Dock or arbitrary third-party application context menus.

## Window tiling at Deskflow edges

A mouse drag stays on its current computer even if **Disable lock to computer
(scroll lock key)** is checked. That option now only disables the explicit cursor
lock. Native macOS tiling remains responsible for arranging the window. Normal
pointer movement still switches computers as before; sending a window's contents
uses the explicit handoff action above.

## Developer checks

`HandoffTests` covers the transfer framing, file contents, interrupted copies,
filename collisions, path validation, URL validation, cancellation and SSH
arguments. On a logged-in Mac, run the real SSH/receiver smoke check with:

```sh
python3 src/unittests/gui/handoff_ssh_smoke.py build/bin/Deskflow.app/Contents/MacOS/deskflow-handoff
```

It runs a temporary SSH server on loopback with disposable keys; it does not
change Remote Login or your SSH configuration. It creates uniquely named copies
in Downloads, checks their contents, then removes those verified test copies.
An optional second argument, such as `com.apple.TextEdit`, also opens a disposable
test document in that installed app. Close that test document afterwards.
