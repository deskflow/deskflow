# Migrating from the external KVM supervisor to native auto mode

How to retire `coordinator.py` / `inputmon` (macOS) and `KvmSwitch.exe` /
`KvmService` (Windows) on a machine and run `deskflow-core auto` instead.

## 1. Settings translation

Legacy `kvm-config.json` / `kvm-config.txt` keys map to Deskflow settings:

| Legacy | Native setting |
|---|---|
| `my_name` | `core/computerName` |
| `coord_port` | `coordination/port` (default 24851) |
| `deskflow_port` | `core/port` |
| `peers[] {name, ip, lan}` | `coordination/peers` = `name=ip\|lan, name2=ip2, ...` |
| (none -- mesh was open) | `coordination/token` (recommended) |
| `mouser_bridge_token/port` | `server/mouserBridgeToken` / `server/mouserBridgePort` |
| `mouser_token/port` | `client/mouserToken` / `client/mouserPort` |

Example settings file for a three-machine cluster:

```ini
[core]
computerName=hackintosh
port=24800

[coordination]
enabled=true
port=24851
peers=macbookpro=100.75.218.20|macbookpro.local, hackintosh=100.126.157.97|hackintosh.local, tiny11=100.90.248.22|tiny11.local
token=<shared secret>

[server]
externalConfig=true
externalConfigFile=/path/to/layout.conf
mouserBridgeEnabled=true
mouserBridgePort=19796
mouserBridgeToken=<mouser bridge secret>

[client]
mouserEnabled=true
mouserPort=19795
mouserToken=<mouser device secret>

[security]
tlsEnabled=false
```

Because roles flip in-process, the Mouser bridge/client keys live in the
one settings file permanently -- the per-flip conf regeneration the legacy
coordinator did is gone.

## 2. Per-machine cutover

1. Stop and disable the legacy supervisor:
   - macOS: `launchctl bootout gui/$UID com.kvm.autoswitch` (and the
     loginwindow agent if installed); remove the plists.
   - Windows: `sc stop KvmService && sc config KvmService start= disabled`.
2. Install the fork build of deskflow-core.
3. Launch `deskflow-core auto -s <settings file>` from your preferred
   keep-alive mechanism (launchd agent / Windows service / deskflow GUI).
   One long-lived process per machine; it elects and flips roles itself.
4. macOS permissions: grant the deskflow-core binary Accessibility and
   Input Monitoring once. Auto mode needs both on every Mac (server
   capture + genuine-input detection).

Mixed clusters work during migration: the wire protocol is identical, so
native nodes and legacy `coordinator.py` nodes elect together — but only
when no `coordination/token` is set (legacy nodes don't send one). Set the
token after the last machine is cut over.

## 3. Operator tooling

`kvmctl status` and `kvmctl primary <name>` work unchanged against native
nodes (same `status` / `promote` messages). If a mesh token is set, add
`"token":"<secret>"` to the JSON those scripts send.

## 4. What stays external

- **Login-window injection** (`deskflow_vhid_bridge` + Karabiner
  DriverKit): still a separate binary; native auto mode covers logged-in
  sessions. (Hook point documented in design.md.)
- **Wake-on-LAN** (`wake-tiny11.sh`): unchanged.

## 5. Known follow-ups

- `cursorHere` strict-burst regime is wired in the coordinator but not yet
  fed by the running client's enter/leave events (Coordinator
  notifyCursorHere is plumbed for it); until then the normal burst
  threshold applies everywhere.
- Windows session/desktop switching (Winlogon vs Default) remains the
  deskflow daemon's domain, as before.
- Run the repo clang-format (scripts/install_deps.py --only-python) over
  src/lib/coordination and src/apps/deskflow-core before opening a PR.
