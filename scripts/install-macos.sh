#!/usr/bin/env bash
# Quit running Deskflow processes, install the built .app to /Applications, restart.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

if [[ -f .env ]]; then
  set -a
  # shellcheck disable=SC1091
  source .env
  set +a
fi

BUILD_DIR="${DESKFLOW_BUILD_DIR:-build}"
INSTALL_APP="${DESKFLOW_INSTALL_APP:-/Applications/Deskflow.app}"
INSTALL_ROOT="$(dirname "$INSTALL_APP")"
APP_NAME="$(basename "$INSTALL_APP" .app)"
SOURCE_APP="$ROOT/$BUILD_DIR/bin/Deskflow.app"
RESTART=1

usage() {
  cat <<'EOF'
Usage: scripts/install-macos.sh [--no-restart] [--install-app PATH]

Quits Deskflow (GUI, deskflow-core, deskflow-vhid-bridge), installs the built
bundle to /Applications/Deskflow.app (or DESKFLOW_INSTALL_APP), clears quarantine,
verifies codesign when possible, and relaunches from the install path.

Requires a prior Release build (build/bin/Deskflow.app or cmake --install).
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --no-restart) RESTART=0; shift ;;
    --install-app)
      INSTALL_APP="$2"
      INSTALL_ROOT="$(dirname "$INSTALL_APP")"
      shift 2
      ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown option: $1" >&2; usage >&2; exit 1 ;;
  esac
done

quit_deskflow() {
  echo "== Stopping Deskflow processes =="
  osascript -e 'tell application "Deskflow" to quit' 2>/dev/null || true

  local -a patterns=(
    deskflow-core
    "${APP_NAME}.app/Contents/MacOS/${APP_NAME}"
    deskflow-vhid-bridge
  )
  for _ in $(seq 1 20); do
    local alive=0
    for pat in "${patterns[@]}"; do
      if pgrep -f "$pat" >/dev/null 2>&1; then
        alive=1
        pkill -f "$pat" 2>/dev/null || true
      fi
    done
    [[ "$alive" -eq 0 ]] && break
    sleep 0.5
  done
  for pat in "${patterns[@]}"; do
    pkill -9 -f "$pat" 2>/dev/null || true
  done

  # Wait until the GUI singleton lock is released before replacing the bundle.
  for _ in $(seq 1 20); do
    if ! pgrep -f "${APP_NAME}.app/Contents/MacOS/${APP_NAME}" >/dev/null 2>&1 &&
       ! pgrep -x deskflow-core >/dev/null 2>&1; then
      return 0
    fi
    sleep 0.5
  done
  echo "warning: some Deskflow processes may still be running" >&2
}

restart_deskflow() {
  [[ "$RESTART" -eq 1 ]] || return 0
  # Brief pause: macOS may relaunch login-item apps after the bundle is replaced.
  sleep 2
  echo "== Launching $INSTALL_APP =="
  # --show brings the window to the front and sets a regular menu-bar presence.
  # If an instance is already running, a second launch pings it via the GUI socket
  # and exits immediately (single-instance), so this is safe after reinstall.
  open "$INSTALL_APP" --args --show
}

install_bundle() {
  echo "== Installing to $INSTALL_APP =="
  if [[ -e "$INSTALL_APP" ]]; then
    rm -rf "${INSTALL_APP}.bak"
    mv "$INSTALL_APP" "${INSTALL_APP}.bak"
    echo "Backed up previous install to ${INSTALL_APP}.bak"
  fi

  if [[ -d "$BUILD_DIR" ]] && [[ -f "$BUILD_DIR/cmake_install.cmake" ]]; then
    local stage
    stage="$(mktemp -d "${TMPDIR:-/tmp}/deskflow-install.XXXXXX")"
    echo "Using staged cmake --install (macdeployqt + bundle layout)"
    cmake --install "$BUILD_DIR" --prefix "$stage"
    if [[ ! -d "$stage/Deskflow.app" ]]; then
      rm -rf "$stage"
      echo "error: staged install did not produce Deskflow.app" >&2
      exit 1
    fi
    cp -R "$stage/Deskflow.app" "$INSTALL_APP"
    rm -rf "$stage"
  elif [[ -d "$SOURCE_APP" ]]; then
    echo "Using build tree copy from $SOURCE_APP"
    cp -R "$SOURCE_APP" "$INSTALL_APP"
  else
    echo "error: no install source — build first (missing $SOURCE_APP and $BUILD_DIR/cmake_install.cmake)" >&2
    exit 1
  fi

  if [[ ! -d "$INSTALL_APP" ]]; then
    echo "error: install failed — $INSTALL_APP not found" >&2
    exit 1
  fi

  xattr -cr "$INSTALL_APP" 2>/dev/null || true

  if codesign --verify --deep --strict "$INSTALL_APP" 2>/dev/null; then
    echo "== Codesign verify OK =="
    codesign -dv "$INSTALL_APP/Contents/MacOS/deskflow-core" 2>&1 | grep -E 'Authority=|TeamIdentifier=' || true
  else
    echo "== Installed unsigned (codesign verify skipped or failed) =="
  fi

  # libqsvgicon.dylib needs QtSvg.framework; without it tray/menu SVG icons are blank
  # in Release installs while Debug (Homebrew Qt) still works.
  if [[ -f "$INSTALL_APP/Contents/PlugIns/iconengines/libqsvgicon.dylib" ]] &&
     [[ ! -d "$INSTALL_APP/Contents/Frameworks/QtSvg.framework" ]]; then
    echo "error: libqsvgicon.dylib is present but QtSvg.framework is missing — rebuild with Qt6::Svg linked" >&2
    exit 1
  fi
}

quit_deskflow
install_bundle
restart_deskflow
echo "== Done: $INSTALL_APP =="
