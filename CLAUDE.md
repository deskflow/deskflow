# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Deskflow is a free and open source keyboard and mouse sharing application (software KVM without video). It allows controlling multiple computers from a single keyboard/mouse with clipboard sharing. Supports Windows, macOS, Linux, and BSD.

- **Language**: C++20
- **Build System**: CMake 3.24+
- **GUI Framework**: Qt 6.7+
- **Network Protocol**: TCP on port 24800, TLS encryption by default
- **Compatible with**: Input Leap, Barrier, and Synergy 1

## Build Commands

```bash
# Configure (Release build)
cmake -Bbuild -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build -j8

# Install (test installation)
DESTDIR=/tmp/install cmake --install build

# Generate packages
cmake --build build --target package
```

### Key CMake Options

- `BUILD_TESTS=ON` - Build unit tests (default ON)
- `SKIP_BUILD_TESTS=OFF` - Skip running tests at build time
- `ENABLE_COVERAGE=OFF` - Enable test coverage
- `BUILD_X11_SUPPORT=ON` - Build X11 backend (Linux/BSD)
- `VCPKG_QT=OFF` - Use Qt from vcpkg (Windows)

## Testing

```bash
# Run unit tests (requires headless display for Qt)
QT_QPA_PLATFORM=offscreen ctest --test-dir build/src/unittests --output-on-failure

# Run legacy tests
./build/bin/legacytests
```

Tests are in `src/unittests/` mirroring the `src/lib/` structure. Uses Google Test framework.

## Code Style

Uses clang-format (LLVM-based style with customizations):
- Column limit: 120 characters
- No single-line functions
- Block indent for function parameters
- Braces on new lines after classes, functions, structs, enums

```bash
# Format a file
clang-format -i <file>
```

CI enforces: clang-format, REUSE license compliance, spell checking (cspell).

## Architecture

### Source Structure

```
src/
├── apps/                  # Executables
│   ├── deskflow-gui/      # Qt GUI application (main entry point)
│   ├── deskflow-core/     # CLI core (no GUI)
│   └── deskflow-daemon/   # Background daemon
├── lib/                   # Core libraries
│   ├── arch/              # Platform abstraction (threading, networking, logging)
│   ├── base/              # Base utilities, exceptions, event system
│   ├── client/            # Client-side logic
│   ├── deskflow/          # Core protocol (KeyMap, KeyState, Screen, Clipboard)
│   ├── gui/               # Qt GUI framework abstractions
│   ├── io/                # Stream operations
│   ├── mt/                # Multi-threading primitives (Lock, CondVar)
│   ├── net/               # Network layer (sockets, TLS, fingerprinting)
│   ├── platform/          # Platform-specific implementations
│   │                      # (Windows, macOS, Linux/Wayland, X11)
│   └── server/            # Server-side logic
└── unittests/             # Tests (mirrors lib/ structure)
```

### Key Components

- **lib/arch**: Platform abstraction layer. All OS-specific operations go through `Arch*` classes.
- **lib/platform**: Platform-specific input/output implementations (MSWindows*, OSX*, Ei* for Wayland, XWindows*).
- **lib/net**: Network communication including `SecureSocket` for TLS.
- **lib/deskflow**: Core protocol logic - `KeyMap` for keyboard mapping, `Screen` for display abstraction, `ProtocolTypes.h` for message definitions.

### Protocol

- Default port: 24800
- TLS enabled by default (since protocol v1.4)
- Primary/Secondary model: Primary shares input, Secondary receives it
- Key files: `ProtocolTypes.h`, `ProtocolUtil.h`

## Platform Requirements

- **Windows**: 10 v1809+, VC++ Redistributable
- **macOS**: 12+ (Intel), 13+ (Apple Silicon), needs accessibility permissions
- **Linux/BSD**: libei 1.3+, libportal 0.8+ for Wayland support

## CI Notes

- Builds with `-DCMAKE_COMPILE_WARNING_AS_ERROR=ON` - all warnings must be fixed
- Tests across 18+ Linux distros, Windows (x64/arm64), macOS (Intel/Apple Silicon), FreeBSD
- Quality gates: lint-reuse, lint-clang, unit tests, legacy tests must all pass
