# Native coordination design

Implements `behavior-spec.md` inside deskflow-core, replacing the external
supervisor stack (`coordinator.py`, `KvmSwitch.cs`/`KvmService.cs`,
`inputmon.swift`). One binary, one settings file, one TCC grant.

## Module layout (`src/lib/coordination/`)

Separation of concerns, dependency-ordered:

| Component | Responsibility | Dependencies |
|---|---|---|
| `Peer` | value type: `{name, ip, lan}` | none |
| `CoordinationProtocol` | encode/decode `claim` / `promote` / `status` JSON lines | QtCore (JSON) |
| `ElectionState` | **pure** election logic: role, seq gossip, cooldowns, input-burst debounce, claim acceptance | none (time injected) |
| `CoordinationMesh` | TCP listener on the coordination port + one-shot sends to peers (thread + BSD sockets, `MouserBridge` pattern) | Protocol |
| `LocalInputMonitor` | platform "genuine hardware input" signal (`ILocalInputMonitor`; macOS CGEventTap source-PID==0, Windows Raw Input `hDevice != NULL`) | platform |
| `Coordinator` | orchestrates the above on the app event queue; emits role-change events; runs the reconciler timer | all of the above + `IEventQueue` |

deskflow-core gains a third positional mode, `auto`, which runs the
**epoch loop**.

## Epoch loop (in-process role flips)

```
coordinator starts mesh + input monitor (process lifetime)
while not quitting:
  role = coordinator.currentRole()          (blocks in init until elected)
  app  = role == Server ? new ServerApp() : new ClientApp()
  app->mainLoop()                           (coordinator posts Quit on flip)
  delete app                                (App singleton freed per epoch)
```

The architecture audit identified the hardening this requires, which is
part of this change:

1. The `App` instance is deleted between epochs (the singleton assert in
   `App::App` then holds).
2. `ServerApp::mainLoop` removes its `ServerAppResetServer` handler on
   exit (pre-existing leak; fixed).
3. Role flips are requested only via an event posted to the queue
   (`EventTypes::CoordinationRoleChange`), so the running app quits at a
   well-defined point in its own loop.

Fallback: if a platform accumulates hidden screen state across epochs,
`RoleController::flip` can switch to self-re-exec (`execv` keeps the PID
and the TCC grant on macOS) without touching any other component. The
epoch loop is the default because it preserves the mesh listener and
input monitor across flips.

## Election semantics

Implemented exactly per `behavior-spec.md` §3–§4, all constants in one
header (`ElectionTuning`):

- `kSelfCooldown` 2.5 s, `kClaimCooldown` 1.5 s, heartbeat 3 s
- burst: 4 events / 0.40 s normally; 12 / 0.80 s while the shared cursor
  is on this screen (the server tells us via enter/leave, which in-process
  are real events rather than parsed log lines)
- same-host heartbeat claims are no-ops
- `seq` gossip via `max()`

`ElectionState` is deterministic and clock-injected: every rule above is
unit-tested without sockets or sleeps.

## Mesh & kvmctl compatibility

Wire format is byte-compatible with the legacy coordinator (§2), so
existing `kvmctl status` / `kvmctl primary` keep working. Additions:

- optional `coordination/token`: when set, messages must carry
  `"token": "<value>"`; non-matching messages are dropped. (The legacy
  mesh was unauthenticated; default stays open for drop-in migration.)
- LAN-first reachability probe before repointing the client (§3.4).

## Reconciler

A 3 s `EventQueueTimer` on the coordinator:

- client link health from real connection events
  (`ClientConnected` / `ClientConnectionFailed` / `ClientDisconnected`)
  with the 10 s grace, then targeted relaunch, then rediscovery
- server wedge probe (`127.0.0.1:<port>` connect, 2 strikes ≈ 18 s)
- startup discovery window (30 s of 1 s `status` polls while `init`)

## Settings

New keys (registered in `common/Settings.h`, defaults in `Settings.cpp`):

| Key | Default | Meaning |
|---|---|---|
| `coordination/enabled` | `false` | master switch for `auto` mode |
| `coordination/port` | `24851` | mesh port |
| `coordination/peers` | `""` | comma-separated peers: bare `name` (resolves `name.local` LAN-first, then `name`) or explicit `name=ip[\|lan]` |
| `coordination/token` | `""` | optional shared mesh token |

Machine name reuses `core/computerName`. The Mouser bridge/client keys
(`server/mouserBridge*`, `client/mouser*`) are read directly by the
respective role at startup — with the epoch loop they no longer need to be
rewritten per flip, which retires the conf-regeneration hack entirely.

## Explicitly out of scope (documented hook points)

- **Login-window injection** (`deskflow_vhid_bridge`): depends on the
  Karabiner DriverKit virtual-HID stack; remains an external binary. The
  coordinator exposes the same client-role hook the legacy system used.
- **Windows service/session management** (`KvmService.cs` token/desktop
  juggling): deskflow's existing daemon owns this domain; coordination
  runs within whatever session the core runs in.
- **Wake-on-LAN**: trivial UDP helper, candidate follow-up.
