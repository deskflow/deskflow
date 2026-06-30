---
title: "plan: KVM transparency — Mouser should not know Deskflow exists"
type: plan
date: 2026-06-30
status: active
---

## KVM transparency roadmap

**Goal:** Remote Mouser opens the Logitech mouse like local USB — same PID,
same hidapi path, same settings replay — with **zero** `remote_device` /
`remote_forward` configuration and no awareness that a KVM moved bytes.

**Non-goal:** Merging Mouser into Deskflow. Transport stays in Deskflow;
Logitech semantics stay in Mouser.

---

## Revised phase order (technical review 2026-06-30)

| Phase | Name | Status |
|-------|------|--------|
| **0** | Stabilize Tier 1 (live soak) | Checklist ready |
| **2a** | Mouser auto-detect + internal shim over existing loopback | **Implemented** |
| **1** | Deskflow DFHR binary sink on single `mouserPort` | **Implemented** |
| **2** | Mouser `DeskflowSinkDevice` + transparent transport UI | **Implemented** (hid_gesture USB path optional) |
| **3** | Deskflow `HidppProbe` — eliminate host Mouser | **Implemented** (macOS/Windows; bridge fallback) |
| **4** | Tier 2 virtual HID (DriverKit) | Deferred |

See `docs/code-review/plan-technical-review-kvm-transparency.md` for review rationale.

---

## Firmware write path (explicit limits)

| Setting action | Focus local (host USB) | Focus remote |
|----------------|------------------------|--------------|
| Wheel invert (firmware) | Host Mouser → physical USB | **Cannot reach physical device** in Tier 1/1.5 |
| Gesture remap | Host pipeline | Remote pipeline (decode in attach) |
| Divert arm (`feat_idx`) | Host discovers | Must be in `connect`/`decode` payload |

Tier 1.5 = gestures + OS-layer invert on remote; firmware invert applies on
whichever machine holds USB (host when local). Tier 2 may add a bidirectional
HID++ write tunnel later.

---

## Wire protocol (single loopback port)

**Port:** existing `client/mouserPort` (default 19795) — no parallel `hidSinkPort`.

**Control plane:** line-delimited JSON (`hello`, `connect`, `disconnect`,
`update_decode`) via `kMsgDMouserData` / MouserClient.

**Data plane:** binary **DFHR** frames (`DFHR` + u16 deviceId + u32 len + payload)
for raw HID reports. JSON `{"type":"report"}` remains supported as compat.

**Discovery:** Deskflow client writes `mouser-sink.json` to AppConfigLocation;
Mouser `core/deskflow_integration.py` auto-starts the sink server.

---

## Phase 0 — Stabilize (now)

- [ ] Live soak: Hackintosh + MacBook (`docs/plan/2026-06-30-phase-0-soak-checklist.md`)
- [ ] Confirm semantic relay disabled (`passthrough_decode_only` or auto host bridge)
- [ ] Tray/status: verify passthrough target in logs (GUI polish deferred)

**Linux host:** bridge fallback only (`StubHidGrabber`); Phases 1–2 target macOS/Windows.

---

## Phase 2a — Mouser auto-detect (done)

- [x] `core/deskflow_integration.py` — manifest + `deskflow.conf` reader
- [x] Engine auto-starts `remote_device` / decode-only `remote_forward`
- [x] `settings.deskflow.auto` default true; opt out with `"auto": false`

---

## Phase 1 — Deskflow generic sink (done)

- [x] `HidSink` DFHR encoder (`src/lib/client/HidSink.*`)
- [x] `MouserClient::deliverReport()` binary hot path
- [x] `mouser-sink.json` manifest (`MouserSinkManifest.*`)
- [x] Unit tests: `HidConsumerTests`

---

## Phase 2 — Transparent ingress (done)

- [x] `core/hid_deskflow_backend.py` — `DeskflowSinkDevice`
- [x] `remote_device` multiplexes JSON + DFHR on one socket
- [x] Transparent transport: `USB Receiver` / `deskflow-shim` (not `remote-virtual`)
- [x] `remote_forward` passthrough-locked when host bridge auto-enabled
- [x] `hid_gesture.py` — Deskflow attach via `_try_connect_deskflow` (main read loop)
- [x] `remote_device` delegates to listener ingress when `transparent_transport`

---

## Phase 3 — Decode without host Mouser (Deskflow)

**Effort:** medium (`src/lib/server/HidppProbe.*` + platform probes).

- [x] `probeHidppDecode()` — IRoot walk for REPROG_V4 `feat_idx` (macOS/Windows)
- [x] Product catalog defaults for `gesture_cid` / `extra_diverts`
- [x] Server attach hook populates `m_hidDecodeCache` before seize defer
- [x] Mouser bridge remains optional fallback (Linux host, probe failure)
- [ ] Host Mouser not required when probe succeeds (verify on live hardware)

---

## Phase 4 — Tier 2 virtual HID (optional, deferred)

- [ ] macOS: extend login-bridge DriverKit path — **not** conflated with desktop injection
- [ ] Windows: evaluate user-mode vhid
- [ ] Mouser: disable shim when OS device present

---

## Configuration target state

**User configures:**

- Deskflow: HID passthrough + auto switch + sharing token (GUI)
- Mouser: **nothing extra** when `settings.deskflow.auto` is true

**User does NOT configure (when auto-detect works):**

- `remote_device.token` / `enabled`
- `remote_forward.passthrough_decode_only`
- Matching tokens across machines manually

---

## Acceptance criteria

### Phase 0 (Tier 1)

- [ ] Gesture swipes fire on focused machine only; host silent
- [ ] Wheel invert reapplies on focus return (host physical path)
- [ ] Deskflow quit → Mouser recovers local USB within 2s
- [ ] No full semantic relay (`should_forward()` false in steady state)

### Phase 2 (transparency)

- [x] DFHR binary delivery on single loopback port
- [x] Auto-start from `mouser-sink.json` / `deskflow.conf`
- [ ] Fresh Mouser default `config.json` works with Deskflow only (needs live soak)
- [ ] Device dropdown shows product name (not "remote virtual")
- [ ] `remote_device.enabled` not required manually

### Phase 4 (OS-level)

- [ ] OS enumerates synthetic Logitech HID (DriverKit / vhid)
- [ ] Options+ compatibility (optional)

---

## Test plan

| Layer | Tests |
|-------|-------|
| Deskflow | `HidConsumerTests` — DFHR encode/deliver |
| Mouser | `test_hid_sink.py`, `test_deskflow_integration.py`, `test_remote_binary_frames.py` |
| Integration | Phase 0 soak checklist (two-machine) |

---

## References

- `docs/hid-passthrough.md`
- `docs/plan/2026-06-30-phase-0-soak-checklist.md`
- `docs/plan/2026-06-30-feat-native-mouse-handoff-plan.md`
- `docs/code-review/plan-technical-review-kvm-transparency.md`
- Mouser: `core/deskflow_integration.py`, `core/hid_sink.py`, `core/hid_deskflow_backend.py`
