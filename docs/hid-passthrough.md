# HID pass-through (generic device channel)

Lets the machine a HID device is *physically attached to* hand the device's
**raw vendor reports** to whichever machine currently has KVM focus, where an
arbitrary consumer interprets them. Deskflow becomes a dumb byte pipe for one
chosen interface; it never learns what a "gesture" is.

This generalizes the Mouser-specific relay (`docs/mouser-bridge.md`, the `DMSR`
decoded-event channel) into a device-agnostic transport. Mouser becomes *one
example* of a consumer; a game controller, drawing tablet, or any device whose
vendor reports Deskflow's standard input path drops can ride the same channel,
given a matching decoder on the far end.

## Two independent concerns

Pass-through and auto-switch are orthogonal — either works without the other,
and each is its own checkbox:

| Concern | Setting root | "What it does" (UI copy) |
|---|---|---|
| **Auto switch** | `coordination/*` | "Make the computer with the active mouse the primary." |
| **Device pass-through** | `passthrough/*` | "Forward this controller's extra buttons to whichever screen is active." |

Auto switch is the election subsystem (`src/lib/coordination/`, see
`docs/coordination/design.md`). Pass-through is described here. They compose
(touch-to-promote *and* device-follows-focus) but neither requires the other:
you can pass a controller through to a fixed client with no election, or
auto-switch with no pass-through.

## The key design decision: seize the vendor interface only

Deskflow already teleports the standard pointer, buttons, scroll, and keyboard
across machines — that is its day job. The *only* thing it drops is the
**vendor-defined HID++ reports** (gesture button, thumb wheel, SmartShift,
rawXY, DPI/mode-shift). So pass-through grabs **only the vendor interface**,
never the standard boot-mouse/keyboard interface.

This single decision is what makes the feature tractable:

- We never try to "detach the system mouse," which is the part every OS
  resists. The pointer keeps flowing through Deskflow's normal KVM path.
- Grabbing the vendor interface is exactly what makes the host's own consumer
  (Mouser, Logitech Options+) see the device as "disconnected" — which is the
  goal: while focus is remote, nothing on the host should be remapping a device
  the user isn't touching.

### The lever (verified in Mouser)

Mouser opens the device **non-exclusively** on macOS
(`core/hid_gesture.py`: `hid_darwin_set_open_exclusive(0)`) and matches it by
`VID + PID + usage_page + usage`. So when Deskflow opens that same interface
with a **seize**, Deskflow wins and Mouser observes the device drop. No
cooperation required from the consumer; no race.

## Grab + hide, per platform

Dynamic, driven by focus (the same `focus`/`local` signal the Mouser bridge
already emits):

- **focus local** (you are on the host): do *not* seize — the host's own
  consumer handles the device normally.
- **focus remote** (cursor is on another screen): **seize** the vendor
  interface and forward its reports to the focused client.

| OS | Seize mechanism | "Hide" level | Release on crash? |
|---|---|---|---|
| **macOS** | `IOHIDDeviceOpen(dev, kIOHIDOptionsTypeSeizeDevice)` on the vendor interface (matched by `VID+PID+usagePage+usage`) | App-level: gesture reports pulled from Mouser **and** Options+ | Yes — OS releases the seize on process exit |
| **Windows** | exclusive `CreateFile(dwShareMode = 0)` on the vendor HID collection | App-level: other openers of that collection are refused | Yes — released on handle close |
| **Linux** | `hidraw` exclusive / `EVIOCGRAB` | App-level (or evdev grab) | Yes — released on fd close |

True OS-level "device vanishes from Device Manager" would need a filter driver
on Windows; we explicitly **do not** require that, because seizing only the
vendor collection already achieves the goal. Crash-safety is inherent: every
platform releases the grab when the Deskflow process dies, so a crash instantly
returns the device to the host.

Release the seize on: focus return to host, pass-through disabled, device
unplugged, or process exit.

## Fail-safe guarantees

The seize lives in Deskflow, not in the consumer — so a consumer (Mouser) on a
machine where Deskflow is absent, disabled, or dead always owns its devices.
The grab happens only when **all three** hold: Deskflow is running, AND
`passthrough/enabled` is set, AND focus is on a remote screen.

| Situation | Seize? | Consumer (e.g. Mouser) on that machine |
|---|---|---|
| Deskflow closed / not installed | never | works normally, standalone |
| Deskflow running, pass-through off | never | works normally |
| Pass-through on, focus **local** | no | works normally |
| Pass-through on, focus **remote** | yes | sees device "disconnect" (intended — the user is not on this machine) |
| Deskflow **crashes** mid-seize | OS auto-releases on process exit | device returns; consumer re-acquires |

These must hold on every platform; a change that violates any row of this
table is a regression, not a tradeoff. The crash row is inherent to the chosen
mechanisms (macOS seize, Windows exclusive handle, Linux fd grab are all
released by the kernel when the process dies) — no cleanup daemon, lock file,
or watchdog is involved, and none may be introduced as a substitute.

Single-PC use is the trivial safe case: with Deskflow not running there is
nothing to seize, and Mouser opens the device exactly as it does today. This
mirrors Mouser's own posture for its bridge (`core/remote_forward.py`: if the
bridge is unreachable, behave "exactly as if this module did not exist").
Consumers never need to know whether Deskflow exists.

## Wire protocol (host → focused client, over Deskflow's connection)

Evolves the `DMSR` channel from a decoded-event vocabulary into raw frames plus
a small control envelope. Carried inside Deskflow's existing (optionally TLS)
transport — no new cross-machine socket.

Control messages (JSON line, like `DMSR` today):

```
attach   { "type": "attach",
           "device": { "vid": "0x046D", "pid": "0xB042",
                       "usage_page": "0xFF43", "usage": "0x0202",
                       "name": "MX Master 4" } }
detach   { "type": "detach", "device": { "pid": "0xB042" } }
```

Data messages (raw input report; **binary**, not JSON — vendor reports are
opaque bytes and base64/hex would bloat the hot path):

```
report   [u16 deviceId][u8 reportId][u16 len][len bytes]
```

`attach` is replayed to a client when it gains focus and `detach` sent when
focus leaves it, mirroring the existing bridge's "virtual device follows
focus" behavior. Reports flow only to the client currently holding focus.

## Client re-expose — two tiers

**Tier 1 (default): app consumer.** The client re-emits attach/report/detach to
a loopback **consumer socket**, token-authenticated, one consumer at a time.
This is the natural evolution of Mouser's `core/remote_device.py`: instead of
receiving decoded events (`gesture_down`), it receives the same **raw report
bytes** its local `hid_gesture` decoder already consumes from a local hidapi
handle. Mouser stays entirely self-contained — it just gets its bytes from a
socket instead of from USB/BT. Lightweight; no driver.

**Tier 2 (future): virtual HID device.** The client re-injects the frames as a
**synthetic HID device** via the Karabiner DriverKit virtual-HID stack (the
same building block as the login-screen bridge, `deskflow_vhid_bridge.cpp`), so
the client OS — and even its own Options+ — sees a real device. This is full
HID-over-IP / "teleport the physical device." Heavier (needs the vhid driver on
the client) and unnecessary for gestures; kept as a later tier, not the first
build.

## Settings

Host (the machine a device is attached to):

```
[passthrough]
enabled = true
devices = 046D:B042            ; VID:PID selectors; or "logitech-vendor" preset
                               ; (optionally /usage=FF43:0202 to pin the interface)
```

Focused client (the consumer side):

```
[passthrough]
sinkEnabled = true
sinkPort    = 19797            ; loopback consumer port
sinkToken   = <shared secret>  ; loopback auth, generated by the UI
```

Both loopback hops are token-gated; the only cross-machine hop rides Deskflow's
existing connection. Same security posture as the Mouser bridge: loopback-only
listeners, constant-time token compare, fail-closed without a token.

## UX (two checkboxes)

In the GUI (`MainWindow` / `SettingsDialog`, system tray already exists):

1. **Auto switch** — "Make the computer with the active mouse the primary."
   Toggles `coordination/enabled` and launches the core in `auto` mode.
2. **Share device controls** — "Forward this controller's extra buttons to the
   active screen." Toggles `passthrough/enabled` and reveals a device picker
   (enumerated local HID, labeled by name). The shared loopback token is
   generated and written to both the host and consumer configs automatically —
   no manual token matching.

Tray status line: role + active pass-through, e.g.
`server · passing through: MX Master 4 → laptop`. The descriptor in the
`attach` message already carries the device name, so the tray needs no
consumer-side help to render this.

## Relationship to the existing Mouser bridge

This **supersedes** the decoded-event `DMSR` path. Migration:

1. Land the raw channel + Tier-1 consumer socket alongside the existing `DMSR`
   relay (both compile, `DMSR` still works).
2. Add a "raw frame" input source to Mouser (`core/remote_device.py`): accept
   report bytes from the socket and feed them into the same `hid_gesture`
   decoder, instead of (or in addition to) decoded events.
3. Once the raw sink is proven, retire the `gesture_down`/`thumb_button_down`
   vocabulary from Deskflow's wire entirely — Deskflow no longer encodes any
   device semantics.

## Mouser bridge dependency (decode sync)

When HID passthrough is enabled on the server, Deskflow automatically starts
the loopback Mouser bridge (`Server::initMouserBridge`) even if legacy gesture
sharing is off. The bridge receives `decode` messages from the host Mouser so
`feat_idx` / `gesture_cid` can be merged into HID connect lines before vendor
interface seize. The Sharing tab hides the legacy gesture-only toggle while HID
passthrough is selected.

## Scope / non-goals

- **Not** re-teleporting the pointer/clicks/keyboard — Deskflow's normal path
  already does that. Pass-through carries only the vendor reports it drops.
- **No kernel driver** required (Tier 1). Tier 2's virtual device reuses the
  existing DriverKit bridge.
- **Transport is generic; intelligence is not.** Raw MX Master frames are
  meaningless unless the far side speaks MX Master HID++. Deskflow stays
  device-agnostic; the decoder stays pluggable on the endpoints.

## Implementation status

Implemented (Tier 1):

1. **Host grab — macOS** — `src/lib/server/OSXHidGrabber.mm`: IOHIDManager discovery
   by VID/PID, vendor-collection filter (usage page >= 0xFF00), focus-driven
   `kIOHIDOptionsTypeSeizeDevice`, input-report callback.
2. **Host grab — Windows** — `src/lib/server/WinHidGrabber.cpp`: SetupAPI enumeration,
   vendor-collection filter, focus-driven exclusive `CreateFile`, overlapped
   `ReadFile`. Linux remains a stub (`StubHidGrabber.cpp`).
3. **Raw channel** — attach/detach ride the existing `DMSR` relay as
   consumer-protocol `connect`/`disconnect` JSON; raw frames travel as the
   new binary `HIDR` message (`kMsgDHidReport`). `Server` follows focus via
   `VirtualHostTracker` (`Server::updateHidVirtualHost`).
3. **Client sink** — `ServerProxy::hidReport()` delivers raw bytes through
   the pluggable `deskflow::client::HidConsumer` interface. The default
   `MouserHidConsumer` adapter re-encodes frames as loopback JSON for Mouser;
   set `client/hidConsumer=none` to drop reports locally.
4. **Mouser raw-frame source** — Mouser's `core/remote_device.py` decodes
   `report` messages with a detached `HidGestureListener` seeded from
   `connect.decode` or `settings.remote_device.decode`
   (`feat_idx`/`gesture_cid`/`extra_diverts`).
5. Settings: `server/hidPassthroughEnabled` + `server/hidPassthroughDevices`
   (`VID:PID` list, `*` PID wildcard); unit-tested selector parsing.

Remaining:

- **Decode-context handoff** — the host's consumer knows `feat_idx` /
  `gesture_cid` (it armed the diverts before the seize); shipping that to
  the focused client automatically (instead of the manual
   `settings.remote_device.decode` override) needs a small host-consumer →
   Deskflow message before the first seize.
- **Linux host grab** (`EVIOCGRAB` / hidraw exclusive).
- **Two-checkbox UX polish** — auto-generated loopback tokens, tray status
  line, device picker UI (selectors are settings-only today).
- **Tier 2** — virtual-HID re-injection on the client via DriverKit.
</content>
</invoke>
