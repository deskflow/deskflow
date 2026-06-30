# Fork roadmap and upstream PR tracker

This fork (`hughesyadaddy/deskflow`, branch `master`) tracks
[`deskflow/deskflow`](https://github.com/deskflow/deskflow) upstream and adds
native auto-switch, HID passthrough, Mouser bridge, and fleet install tooling.

**Upstream sync:** merge `upstream/master` into `master` weekly. As of 2026-06-30,
local `master` is current with upstream (merge `6afa46dca`).

## Documentation index

| Topic | Doc |
|-------|-----|
| Native coordination / auto-switch | `docs/coordination/design.md`, `docs/coordination/migration.md` |
| HID device passthrough | `docs/hid-passthrough.md` |
| Logitech Mouser bridge (legacy relay) | `docs/mouser-bridge.md` |
| macOS login-screen injection | `docs/auto-switch-design.md` |
| Fleet install | `INSTALL.md`, `docs/building-signed.md` |
| Local debug (VS Code / Cursor) | `docs/dev/MACOS_DEBUG.md` |
| Consolidation plan | `docs/plan/2026-06-30-refactor-fork-consolidation-upstream-readiness-plan.md` |

## Upstream PR stack

Open a [discussion on deskflow/deskflow](https://github.com/deskflow/deskflow/discussions)
before large features (coordination, HID passthrough).

| PR | Branch | Scope | Status |
|----|--------|-------|--------|
| PR-1 | `fix/posix-recursive-mutex` | `recursive_mutex` for `m_threadMutex` | Not started |
| PR-2 | `fix/macos-poll-active-group` | Main-queue fix for `pollActiveGroup` | Not started |
| PR-3 | `fix/arch-worker-exceptions` | Contain worker-thread exceptions | Not started |
| PR-4 | `fix/client-connect-logging` | Info-level connect/disconnect logs | Not started |
| PR-5 | `fix/core-qapplication-auto` | Single `QApplication` in auto mode | **Done on master** |
| PR-6 | `feat/mouser-bridge` | Mouser JSON relay | Not started |
| PR-7 | `feat/hid-passthrough-tier1` | HID seize + relay | Not started |
| PR-8 | `feat/coordination-core` | Election + mesh + auto mode | Not started |
| PR-9 | `feat/auto-mode-gui` | GUI auto-switch UI | Not started |
| PR-10 | `feat/macos-onboarding` | Accessibility gate, SMAppService | Not started |

### Fork-only (not targeted at upstream unless requested)

- `scripts/install-macos.sh`, `scripts/install-windows.ps1`, signing `.env` workflow
- `docs/dev/*` debug launcher examples
- macOS Karabiner login bridge (`deskflow-vhid-bridge`)
- Windows kernel vhid driver + UAC auto-elevate

## Redundancy cleanup (planned)

See consolidation plan Phase 3:

1. Extract `VirtualHostTracker` from `Server.cpp`
2. Tier Mouser bridge as HID decode-sync helper only
3. `HidConsumer` interface on client (decouple from Mouser JSON)
4. Optional rename: macOS `deskflow-vhid-bridge` → `deskflow-login-bridge`

## Superseded branches (safe to delete)

These branches were merged or rebased into `master`:

- `feat/native-coordination`
- `feat/auto-switch`
- `feat/mouser-bridge`
- `claude/karibeener-extension-install-mggwmq`
