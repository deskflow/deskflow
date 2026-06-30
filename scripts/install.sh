#!/usr/bin/env bash
# Cross-platform entry point: macOS install script; use install-windows.ps1 on Windows.
set -euo pipefail

case "$(uname -s)" in
  Darwin)
    exec "$(dirname "$0")/install-macos.sh" "$@"
    ;;
  *)
    echo "On Windows run: pwsh scripts/install-windows.ps1" >&2
    echo "On macOS this script delegates to scripts/install-macos.sh" >&2
    exit 1
    ;;
esac
