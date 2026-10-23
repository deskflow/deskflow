<!-- SPDX-FileCopyrightText: (C) 2026 Deskflow Developers -->
<!-- SPDX-License-Identifier: MIT -->

# macOS input-source concurrency regression

## Failure mechanism

On a macOS server, an abort report can show AppKit's
`UpdateKeyboardAwareShortcutsForCurrentInputSource` on the main thread while
`OSXKeyState::pollActiveGroup` calls `TISCopyCurrentKeyboardLayoutInputSource`
on a worker thread. A mutex used only by Deskflow does not serialize AppKit's
independent TIS calls. This is distinct from proving that a particular input
method causes the failure.

The candidate dispatches the small TIS operations to the main queue, running
inline when already on the main thread. No Deskflow TIS mutex is held while
waiting. Keyboard translation and its return values remain synchronous.
This design still requires a responsive main event loop; it is not a claim
that every event-tap, shutdown, or input-source switching scenario is covered.

## Read-only reproduction

Build the macOS tests using the normal project build configuration:

```sh
cmake --build build --target TISMainThreadProbe TISReadPathsTests
ctest --test-dir build/src/unittests \
  -R '^(TISMainThreadProbe|TISReadPathsTests)$' --output-on-failure
```

`TISMainThreadProbe` uses four worker threads and an independent main-thread
reader, modeling AppKit's calls without interacting with another application.
Its default mode checks main-thread affinity, nested calls, a move-only closure,
exception propagation, and 10,000 worker operations.

To model the old Deskflow-only mutex approach, run the probe separately:

```sh
build/src/unittests/platform/TISMainThreadProbe --baseline-mutex
```

**The baseline mode can intentionally abort its own test process.** It was
observed to produce the same concurrent TIS/TSM assertion. It is not registered
as an automated crashing test. Do not run a second Deskflow server to reproduce
this while using an existing connection.

`TISReadPathsTests` calls the real key-map update, active group, current language,
layout list, legacy character conversion, and in-memory key translation paths
from four workers. The 4,000 iterations compare returned values with main-thread
reference results. The test uses temporary settings/state directories, does not
create an event tap, post a key, select an input source, or open a clipboard.
An input-source change by the user during the test can invalidate its reference
values; the test does not force an input source to avoid that.

## Evidence and limits

The v1.27.0-based candidate was built on Apple Silicon, macOS 26.6.2, Qt 6.11.3.
Ten fresh probe runs (100,000 worker operations) and ten real-path runs
(40,000 iteration groups) completed successfully. Local clipboard serialization
and chunk tests passed. A real macOS-server/Windows-client smoke test confirmed
pointer crossing and typing; a separately managed controlled core-abort drill
restored its TLS connection without manual intervention.

Those live results apply to the v1.27.0-based local build, not automatically to
every newer upstream revision. The PR branch, based on upstream `c1b19244`, was
separately built on the same environment: core and both new test targets compiled;
both new tests and the clipboard serialization/chunk tests each passed ten CTest
runs. The probe's separate baseline mode exited with SIGABRT. The modifier-map
test was run explicitly without the upstream tests that post real keys.
The fault drill tests recovery from an injected abort, not reproduction of a
natural TIS race in the patched build. Independent clipboard-provider smoke tests
do not establish that Deskflow's native clipboard implementation is fixed.

Not covered by those results: a macOS client role, real input-source selection,
sleep/wake, lock/unlock, reboot, shutdown while a callback is pending, busy-main-
thread tail latency, and Windows/Linux builds. No original crash reports,
hostnames, network addresses, configuration, certificates or clipboard contents
are included here.
