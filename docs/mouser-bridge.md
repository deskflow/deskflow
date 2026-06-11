# Mouser bridge (fork extension)

Forwards Logitech HID++ gestures and buttons from the machine the mouse is
physically attached to (the Deskflow **server**) to whichever machine has
KVM focus, where the local [Mouser](https://github.com/hughesyadaddy/Mouser)
instance executes them natively. No HID emulation: device identity and
events travel as data and feed Mouser's existing pipeline.

## Flow

```
machine A (server, mouse attached)            machine B (client, has focus)
┌─────────────┐  loopback   ┌──────────┐ DMSR ┌──────────┐ loopback ┌─────────────┐
│  Mouser A    ├────────────►│ Deskflow │─────►│ Deskflow │─────────►│  Mouser B    │
│ (forwarder)  │◄────────────┤  server  │      │  client  │          │ (remote dev) │
└─────────────┘  focus msgs  └──────────┘      └──────────┘          └─────────────┘
```

- **MouserBridge** (server): loopback TCP listener (`server/mouserBridgePort`,
  default 19796). Mouser A connects, authenticates with
  `server/mouserBridgeToken`, sends device-identity + event JSON lines, and
  receives `{"type": "focus", ...}` notifications on every screen switch.
- **DMSR protocol message** (`"DMSR%s"`, server → client): one Mouser
  JSON line relayed verbatim. Only ever sent when the bridge is enabled;
  stock clients never see it.
- **MouserClient** (client): forwards DMSR payloads into the local Mouser's
  remote-device port (`client/mouserPort`, default 19795) after a hello
  handshake with `client/mouserToken`, on a worker thread so a slow or
  absent Mouser can never stall input processing.

The server caches the last device-connect line and moves the "virtual
device" with focus: connect is replayed to a client when it gains focus,
disconnect is sent when focus leaves it, and events are only relayed to the
client currently hosting the device. If the local Mouser instance vanishes,
a synthetic disconnect clears the virtual device everywhere.

## Settings

Server machine (`deskflow.conf` or Settings file):

```
[server]
mouserBridgeEnabled=true
mouserBridgePort=19796
mouserBridgeToken=<shared secret with Mouser A's settings.remote_forward.token>
```

Client machines:

```
[client]
mouserEnabled=true
mouserPort=19795
mouserToken=<shared secret with Mouser B's settings.remote_device.token>
```

Mouser side: `settings.remote_forward` (machine A) and
`settings.remote_device` (machine B) in Mouser's `config.json`; both are
off by default and require tokens.

## Security posture

- Both listeners/connectors are loopback-only; the only cross-machine hop
  rides Deskflow's existing (optionally TLS) connection.
- Token required on both local hops; bridge refuses to start without one.
- The relay carries an allowlisted event vocabulary (Mouser side enforces
  it); there is no generic input-injection path.
