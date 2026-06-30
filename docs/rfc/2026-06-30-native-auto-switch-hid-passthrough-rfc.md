# RFC: Native auto-switch coordination + HID device passthrough

**Status:** Draft (fork implementation exists; seeking upstream maintainer feedback)  
**Fork:** https://github.com/hughesyadaddy/deskflow  
**Design docs:** `docs/coordination/design.md`, `docs/hid-passthrough.md`, `docs/auto-switch-design.md`

## Summary

We have a working fork that adds two major capabilities to Deskflow:

1. **Native auto-switch** — in-process role election (`deskflow-core auto`) replacing external supervisor scripts (`coordinator.py`, `KvmSwitch.exe`). Peers coordinate over a TCP mesh (port 24851, kvmctl-compatible JSON).

2. **HID device passthrough** — host-side vendor-interface seize and raw report relay (`HIDR` wire message) so Logitech HID++ gestures (and other vendor reports) follow KVM focus without Deskflow learning gesture semantics.

These compose but are independent toggles.

## Motivation

- Multi-machine KVM (Hackintosh + MacBook + Windows) with one physical Logitech mouse
- Gestures must fire on the focused machine only; host Mouser must release the vendor interface when focus is remote
- External Python/Swift/C# supervisors are fragile; want election + transport inside Deskflow

## Proposed upstream scope (phased)

| Phase | Scope | Size | Notes |
|-------|-------|------|-------|
| A | Bug-fix PRs (mutex, pollActiveGroup, logging) | Small | **PRs #9908–#9911 open** |
| B | Mouser bridge (loopback JSON relay) | Medium | Legacy path; may deprecate when HID passthrough matures |
| C | HID passthrough tier 1 (macOS/Win grabbers + `HIDR`) | Large | Preferred end state for vendor reports |
| D | Coordination core (election + mesh + auto mode) | Large | Needs RFC buy-in |
| E | GUI auto-switch UI | Medium | Depends on D |
| F | macOS login-screen bridge (Karabiner) | Niche | Likely fork-only unless requested |

## Out of scope for initial upstream

- Windows kernel vhid driver + UAC auto-elevate stack
- Fleet install/signing scripts (`.env`, `install-macos.sh`)
- VS Code debug launcher (`docs/dev/`)

## Questions for maintainers

1. Appetite for in-process **auto mode** vs keeping server/client as separate long-lived processes?
2. Acceptable to add **HID vendor-interface seize** on macOS (IOHID) and Windows (SetupAPI)?
3. Should Mouser-bridge-style loopback relay merge before or after HID passthrough?
4. Minimum test/CI bar for platform-specific grabbers (Linux stub OK initially)?

## Reference implementation

~55 commits ahead of upstream on fork `master` (as of 2026-06-30). Key paths:

- `src/lib/coordination/` — election, mesh, coordinator
- `src/lib/server/HidPassthrough.*`, `*HidGrabber.*`
- `src/lib/server/MouserBridge.*`
- `src/lib/client/HidConsumer.*`
- `src/apps/deskflow-core/AutoModeRunner.*`

Unit tests: `ElectionStateTests`, `CoordinationProtocolTests`, `HidPassthroughTests`, `VirtualHostTrackerTests`, `HidConsumerTests`.
