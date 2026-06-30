# Technical review: KVM transparency roadmap

**Plan:** `docs/plan/2026-06-30-kvm-transparency-roadmap-plan.md`  
**Reviewer:** technical review (2026-06-30)  
**Verdict:** **Approve with revisions** — direction is correct; phase scope, firmware semantics, and settings migration need tightening before build.

---

## Executive summary

The plan correctly identifies that **only Layer C (HID passthrough)** couples Deskflow and Mouser, and that the user's ideal ("fake Logitech ID, Mouser never knows KVM") is **not met by Tier 1** but **is achievable** via Tier 1.5 (hidapi shim) or Tier 2 (virtual HID).

**Recommended path unchanged:** Phase 0 soak → Phase 1 generic sink (Deskflow) → Phase 2 hidapi shim (Mouser) → Phase 3 decode probe (Deskflow) → Phase 4 virtual HID (optional).

**Block before Phase 1:** Complete Phase 0 live soak on Tier 1; otherwise shim work may paper over transport bugs.

---

## Critical findings (must fix in plan)

### C1 — Acceptance criteria mix phases without labeling

The plan lists criteria like "default `config.json` works" and "no `remote_device.enabled`" under a single **Acceptance criteria (transparency)** section, but those only become true after **Phase 2**. Criteria like "gestures on focused machine only" are true after **Phase 0**.

**Fix:** Split into `Phase 0 (Tier 1)`, `Phase 2 (transparency)`, `Phase 4 (OS-level)`.
 
### C2 — Firmware / settings semantics underspecified

The goal says "same hidapi path, same settings replay" but the plan never states:

| Setting action | Focus local (host) | Focus remote |
|----------------|-------------------|--------------|
| Wheel invert (firmware) | Host Mouser → physical USB | **Cannot reach physical device** in Tier 1/1.5 |
| Gesture remap | Host pipeline | Remote pipeline (decode only) |
| Divert arm (`feat_idx`) | Host discovers | Must be in `attach`/`decode` payload |

Remote focus **cannot** apply firmware invert to the physical mouse without a **bidirectional HID++ write tunnel** (not in any phase). Users may expect per-machine firmware invert **while focused remote** — that only works with OS-layer fallback on remote or Tier 2 + write path.

**Fix:** Add explicit "Firmware write path" subsection: Tier 1.5 = gestures + OS invert on remote; firmware invert applies on whichever machine holds USB (host when local). Tier 2 may add write tunnel later.

### C3 — Phase 1 re-introduces abstraction the consolidation sprint removed

`CONSOLIDATED.md` (I8) collapsed `HidConsumerMode` → `mouserHidDeliveryEnabled()` because Mouser JSON was the only consumer. Phase 1 proposes `HidSink` + parallel `hidSinkPort` / `hidSinkToken`.

**Risk:** Two ports, two tokens, GUI complexity regression.

**Fix:** Prefer **one loopback port, binary-framed protocol** (or extend `HIDR` to loopback as-is) instead of parallel `hidSinkPort`. Keep `encodeHidReportAsMouserLine` as compat shim inside Mouser during migration, not as Deskflow's primary sink API.

---

## Important findings (should fix in plan)

### I1 — Phase 2 shim insertion point needs a concrete hook

Plan says "inside `hid_gesture.py` at open/read boundary" but `HidGestureListener.start()` enumerates real USB via `_vendor_hid_infos()`. The shim needs one of:

1. **Synthetic enumerate entry** (fake path `deskflow://127.0.0.1:PORT`) prepended when Deskflow client is connected, or  
2. **Bypass enumerate** when env `MOUSER_DESKFLOW_SINK=1` or auto-detect loopback server.

Existing patterns: `_HidDeviceCompat`, `_MacNativeHidDevice` — add `_DeskflowSinkDevice` with `read()` blocking on socket queue.

**Fix:** Name the class, hook site (`_open_device` / read thread), and precedence rule (sink vs physical USB on remote — physical should win if present).

### I2 — Phase 3 effort underestimated

"Small HID++ probe module" duplicates `hid_gesture.py` REPROG_V4 discovery: receiver PID, multi `dev_idx`, gesture CID selection, `extra_diverts`. Consolidation sprint kept host Mouser for decode sync **intentionally**.

**Fix:** Phase 3 estimate: **medium-large Deskflow** (new `src/lib/server/HidppProbe.*`), not a small task. Keep host Mouser decode-only until Phase 3 is proven.

### I3 — Phase ordering: Mouser shim before decode independence

Phase 2 (shim) can consume today's `connect` + `decode` over the existing Mouser JSON line format internally — **Phase 1 does not need to block Phase 2** if Mouser wraps current `remote_device` protocol inside the shim first, then Deskflow genericizes later.

**Fix:** Allow **2a** (Mouser internal shim over existing loopback) before **1** (Deskflow generic sink) to reduce cross-repo coupling risk.

### I4 — Linux host gap

`StubHidGrabber` means Linux host still needs semantic relay or no passthrough. Plan mentions Path 3 status quo but not in phased checklist.

**Fix:** Add Phase 0 note: Linux host = bridge fallback only; transparency phases are macOS/Windows client + host grabber platforms.

### I5 — `should_forward()` deprecation timing

Acceptance says "no `should_forward()` in steady state" but semantic relay remains until explicitly removed. Phase 2 should include **delete or hard-disable** `remote_forward` full relay when passthrough detected.

### I6 — Test plan missing

No tests listed for Phase 1–2. Minimum:

- Deskflow: sink delivers raw bytes; attach/detach on focus
- Mouser: shim `read()` feeds `_on_report`; enumeration prefers sink when configured
- Integration: loopback HIDR → shim → gesture swipe dispatch (no USB)

---

## Suggestions

### S1 — Rename phases for clarity

| Current | Suggested |
|---------|-----------|
| Phase 1 Generic sink | Deskflow: device-agnostic loopback transport |
| Phase 2 hidapi shim | Mouser: transparent ingress adapter |
| Phase 3 Decode probe | Deskflow: eliminate host Mouser dependency |
| Phase 4 Tier 2 | Deskflow: virtual HID (OS-level transparency) |

### S2 — Upstream narrative

Generic sink + binary framing is **more upstreamable** than Mouser JSON in Deskflow core. Phase 1 aligns with RFC Phase C; Mouser-specific consumer moves to application layer. Good for deskflow#9913.

### S3 — Tier 2 scope guard

Phase 4 should reference existing `deskflow-vhid-bridge` (login screen only today) and explicitly **not** conflate login-bridge with focused-desktop injection until requirements are separate.

### S4 — Auto-detect over env vars

Prefer Deskflow writing a well-known loopback port into a shared config snippet both apps read, over `MOUSER_HID_SOURCE=deskflow://...` env vars, for fleet install.

---

## Path comparison (validated)

| Path | Mouser KVM-aware? | OS sees Logitech? | Firmware writes remote | Effort |
|------|-------------------|-------------------|------------------------|--------|
| Tier 1 (today) | Yes | No | No | Done |
| Tier 1.5 (shim) | No (user-facing) | No | No | Medium |
| Tier 2 (vhid) | No | Yes | Possible with write tunnel | Large |

**User's "fake each ID"** = Tier 2 at OS level, or Tier 1.5 at application level (Mouser cannot distinguish shim from USB in its own code).

---

## Recommended plan edits (checklist)

- [x] Split acceptance criteria by phase
- [x] Document firmware/write limitations per focus state
- [x] Phase 1: single port + binary framing; avoid parallel token axis
- [x] Phase 2a: Mouser shim over existing protocol before Deskflow generic sink
- [x] Phase 2: specify `_DeskflowSinkDevice` + enumeration precedence
- [x] Phase 3: realistic effort; keep host Mouser until proven (skeleton landed)
- [x] Add integration test section
- [x] Linux host explicit non-goal for Phases 1–2

---

## Go / no-go

| Gate | Status |
|------|--------|
| Architecture direction | **Go** |
| Separation of concerns (A/B vs C) | **Go** |
| Phase 0 complete before Phase 2 | **Required** |
| Phase 1 as written (dual port) | **Revise** |
| Phase 4 in same initiative | **Defer** (optional track) |

**Overall:** Proceed with **revised Phase 1/2** after Phase 0 soak.
