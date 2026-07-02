# Coordination behavior specification

Source-of-truth behavioral spec for the native coordination subsystem
(`src/lib/coordination/`), reverse-engineered from the external
supervisor system it replaces (`coordinator.py` on macOS, `KvmSwitch.cs` +
`KvmService.cs` on Windows, `inputmon.swift`, `deskflow_vhid_bridge.cpp`).
Anything the native implementation does differently from this document is
either a bug or must be called out explicitly in `design.md`.

## 1. System overview

A **touch-to-promote distributed role coordinator**:

- Any machine receiving **genuine local hardware input** claims primary.
- The primary runs Deskflow in **server mode**; all others run **client
  mode** targeting the primary.
- Coordination is newline-delimited JSON over TCP on the coordination port
  (deployed: 24851). Deskflow transport stays on 24800.
- macOS additionally has a login-window context where normal client
  injection (CGEventPost) cannot work and a Karabiner virtual-HID bridge
  is used instead.

## 2. Wire protocol (TCP, newline JSON, one connect per send)

### `claim` — broadcast by current/aspiring primary

```json
{"t": "claim", "name": "<sender>", "ip": "<addr>", "lan": "<lan-addr>", "seq": 7}
```

Sent on local promotion (before becoming server) and as a heartbeat every
3 s while server, to every peer at both its `ip` and `lan` addresses.

### `promote` — manual override

```json
{"t": "promote"}
```

Receiver promotes itself: updates last-switch timestamp, broadcasts a
claim, becomes server. (`kvmctl primary <name>` sends this.)

### `status` — request/response

```json
{"t": "status"}
{"role": "server|client|init", "server_ip": "<addr|null>", "seq": 7,
 "last_switch": 1781148503.01, "name": "<machine>"}
```

Used for startup discovery and operator status.

No auth, no TLS on this mesh today; the native implementation keeps the
wire format but should add an optional shared token (documented in
`design.md`).

## 3. Election semantics

### Genuine-input detection

- **macOS**: listen-only CGEventTap; an event is genuine hardware iff its
  event-source unix PID is 0 (synthesized/injected events carry the
  injecting process's PID). Requires Input Monitoring permission. The tap
  must self-re-enable after timeout disables.
- **Windows**: Raw Input (`WM_INPUT`, `RIDEV_INPUTSINK`); genuine iff
  `RAWINPUTHEADER.hDevice != NULL` (SendInput-synthesized input has a
  null device).

### Debounce / anti-flap

- `SELF_COOLDOWN = 2.5 s`: ignore local promotion triggers right after a
  role change.
- Input burst to promote: >= 4 genuine events within 0.40 s normally; when
  the shared cursor is currently ON this screen (entered via KVM),
  >= 12 events within 0.80 s (prevents forwarded-motion misfires).
- Cursor-on-this-screen is tracked from the client's own "entering
  screen" / "leaving screen" transitions.
- While server: ignore inbound claims for `COOLDOWN = 1.5 s` after the
  flip.
- While client of host X: heartbeat claims for X (by `ip` or `lan`) are
  no-ops (no probing, no relaunch).

### Sequence numbers

Monotonic per-cluster gossip: local broadcast increments `seq`; on receive
`seq = max(local, incoming)`. Not used as a strict tie-break.

### LAN-first targeting

When a claim carries both addresses, probe `lan` on the coordination port
(~0.7 s timeout) and use it for the Deskflow connection when reachable;
fall back to `ip` otherwise.

## 4. Role state machine

States: `init`, `client(server)`, `server`.

| From | To | Trigger |
|---|---|---|
| init | client(X) | inbound claim from X, or startup discovery finds X |
| init | server | genuine-input burst, or `promote` |
| client(X) | server | genuine-input burst, or `promote` |
| server | client(X) | accepted claim from X outside cooldown |
| client(X) | client(X) | no-op (same-host claim) |

Becoming **server**: stop current transport, clear cursor-here, start
server transport, mark link up, persist state.
Becoming **client(X)**: stop current transport, clear link/cursor flags,
start client transport towards X, persist state.

## 5. Reconciler (every 3 s, exception-isolated)

- **Client link health** is event-driven from the transport's own
  lifecycle ("connected to server" => up; "failed to connect" /
  "disconnected" / "server stopped" => down). In-process, these map to
  real events instead of log parsing.
- Client down < 10 s: grace. Beyond that: relaunch towards the same host;
  after >= 2 direct retries, re-run discovery (`status` to peers) and
  repoint if the server is reachable at an alternate address.
- **Server wedge probe**: every ~9 s, connect to `127.0.0.1:24800` with a
  1 s timeout; two consecutive failures (~18 s) => the server is alive but
  not accepting => restart server transport.
- **Startup discovery**: for 30 s while `init`, query peers' `status`
  every 1 s; converge to an active server immediately when found.

## 6. Login-window path (macOS)

At the login window CGEventPost cannot inject, so the client role is
served by a Karabiner DriverKit virtual-HID bridge speaking the Deskflow
protocol (v1.8) directly: handshake, keepalive echo, screen-info reply,
mouse/keyboard via virtual HID, pointer acceleration disabled, fallback
geometry + `login_scale` from config, relative motion via corner-slam +
delta stepping. Non-input protocol messages are ignored. Server role at
the login window uses the normal server transport.

## 7. Windows service specifics

- A LocalSystem service owns process lifecycle; every 1 s it checks the
  active console session and whether the target desktop is
  `WinSta0\Winlogon` (locked / no user) or `WinSta0\Default`, relaunching
  the helper and the core when the target changes or a process died.
- On the user desktop it duplicates the user token and sets
  `TokenUIAccess=1` (elevated-window control + clipboard); on the secure
  desktop it falls back to a SYSTEM token bound to the session.
- Client link health via `GetExtendedTcpTable` (ESTABLISHED to the server
  port); 12 s disconnected => relaunch.

## 8. Configuration (legacy keys -> native settings)

| Legacy (kvm-config) | Meaning |
|---|---|
| `my_name` | this node's peer name |
| `coord_port` (24851) | coordination mesh port |
| `deskflow_port` (24800) | Deskflow transport port |
| `peers[] {name, ip, lan}` | cluster members (includes self) |
| `screen_w/screen_h`, `login_scale` | login-window fallback geometry |
| `mouser_bridge_token/port` | server-side Mouser bridge (see mouser-bridge.md) |
| `mouser_token/port` | client-side Mouser forward |

## 9. Special features to preserve

- Wake-on-LAN for sleeping peers (UDP magic packet, ports 9 and 7).
- `kvmctl status` / `kvmctl primary <name>` operator interface
  (the wire protocol above is the contract).
- Atomic state persistence (write-temp + rename) for supervisors/operators.
- Heartbeat claims must stay no-ops on already-converged clients
  (historic backlog-storm bug).

## 10. Porting-critical requirements

1. Last-touch-wins promotion with sustained-input debounce.
2. Identical `claim`/`promote`/`status` JSON shapes (kvmctl compatibility).
3. Cooldown guards and 3 s heartbeat cadence.
4. LAN-first dual-address targeting with reachability probe.
5. Event-driven link health + reconciler fallback + server wedge probe.
6. Login-window injection fallback hook point.
7. Mouser bridge/client settings follow the active role (now native:
   the in-process role controller configures them directly instead of
   regenerating conf files).

## 11. Mesh v2 extensions (`coordination/meshVersion=2`)

Production default is mesh v2 (`meshVersion=2`). Set `coordination/meshVersion=1`
only for emergency rollback while upgrading a mixed fleet.

### `hello` — capability handshake

```json
{"t": "hello", "v": 2, "name": "<sender>", "token": "<optional>"}
```

Receiver replies with its own `hello` when `meshVersion=2`. v2 nodes reject
peers that advertise `v` < 2. The GUI status poll surfaces mismatches via
`version_mismatch` in the localhost status reply.

### `fleet` — server-authoritative state fragment

```json
{
  "t": "fleet",
  "seq": 12,
  "token": "<optional>",
  "server": "<elected-server-name>",
  "cursor": {"host": "<cursor-host>", "screen": "<screen-name>"},
  "peers": [{"name": "a", "ip": "10.0.0.1", "lan": "a.local"}],
  "links": [{"from": "a", "to": "b", "dir": "right"}],
  "screens": ["a", "b"]
}
```

Clients merge fragments with monotonic `seq` ordering (`FleetStateMerge`).
The elected server is authoritative: each newer fragment replaces
`peers[]`, `links[]`, `screens[]`, and cursor fields. Post-merge, the
coordinator emits `CoordinationFleetStateChanged`; the first non-empty
`links[]` also emits `CoordinationTopologyReady`.

Legacy `cursor` and `keyfwd` messages are rejected on mesh v2 nodes. Roll back
to `coordination/meshVersion=1` on every machine if a straggler cannot upgrade.
