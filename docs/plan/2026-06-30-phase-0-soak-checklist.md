# Phase 0 live soak checklist

Two-machine validation before further transparency work. Requires rebuilt
Deskflow (`master`) and Mouser (`working`) with DFHR + auto-detect.

## Prerequisites

- [ ] macOS TCC: Accessibility for `deskflow-core` and Mouser on both machines
- [ ] Deskflow sharing secret matches on server + clients (GUI → Sharing)
- [ ] HID passthrough enabled on server; `046D:B042` (or your PID) in device list
- [ ] Mouser default `config.json` — **no** manual `remote_device` / `remote_forward`

## Host (mouse attached)

Deskflow `deskflow.conf`:

```ini
[server]
hidPassthroughEnabled=true
hidPassthroughDevices=046D:B042
mouserBridgeEnabled=true
mouserBridgePort=19796
mouserBridgeToken=<secret>
```

Mouser: leave defaults; `settings.deskflow.auto=true` auto-enables decode bridge.

**Host with successful Deskflow HID++ probe:** host Mouser / `remote_forward` not required.
**Linux host:** probe unavailable — keep decode bridge or manual decode override.

## Remote (focused client)

```ini
[client]
mouserEnabled=true
mouserPort=19795
mouserToken=<secret>
```

After Deskflow client connects, verify `~/Library/Application Support/Deskflow/mouser-sink.json` exists.

## Soak steps

1. [ ] Focus local on host — gestures work; host Mouser holds USB
2. [ ] Move focus to remote — host `[HidGesture]` disconnect; remote virtual connect
3. [ ] Swipe gesture on remote — configured action fires (Mission Control, etc.)
4. [ ] Scroll wheel — KVM scroll; invert unchanged on remote (firmware is host-only)
5. [ ] Return focus to host — firmware invert + gestures restore within 2s
6. [ ] `killall deskflow-core` on host — Mouser recovers USB; no ghost device
7. [ ] Deskflow restart — decode republish; remote gestures still work
8. [ ] Confirm logs show `Deskflow HID sink auto-enabled` on remote (no manual KVM config)

## Failure triage

| Symptom | Check |
|---------|--------|
| Remote gestures dead | `connect` includes `decode.feat_idx`; host bridge connected |
| Host fires gestures while remote focused | Passthrough seize active; host Mouser disconnected |
| Token errors | `mouserToken` = Mouser sink token; bridge token on host |
| Binary reports ignored | Deskflow build includes DFHR; Mouser `test_remote_binary_frames` passes |

## Sign-off

- [ ] 30+ minutes continuous use without stuck focus or ghost device
- [ ] Phase 0 criteria in `2026-06-30-kvm-transparency-roadmap-plan.md` met
