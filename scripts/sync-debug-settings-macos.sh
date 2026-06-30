#!/usr/bin/env bash
# Ensure debug sessions use the same Deskflow connection settings as the
# installed app: Deskflow.conf, deskflow-server.conf, and TLS under
# ~/Library/Deskflow (override with DESKFLOW_SETTINGS_DIR).
#
# Auto mode requires coordination/peers. If missing, derive names from
# deskflow-server.conf screen layout or [screen_*] sections in Deskflow.conf.

set -euo pipefail

SETTINGS_DIR="${DESKFLOW_SETTINGS_DIR:-${HOME}/Library/Deskflow}"
CONF="${SETTINGS_DIR}/Deskflow.conf"
SERVER_CONF="${SETTINGS_DIR}/deskflow-server.conf"

if [[ ! -f "${CONF}" ]]; then
  echo "sync-debug-settings: no settings at ${CONF} (skipping)"
  exit 0
fi

peers_from_server_conf() {
  [[ -f "${SERVER_CONF}" ]] || return 1
  awk '
    /^section: screens$/ { in_screens = 1; next }
    /^end$/ && in_screens { exit }
    in_screens && $0 ~ /^\t[a-zA-Z0-9_.-]+:$/ {
      name = $1
      sub(/:$/, "", name)
      sub(/^\t/, "", name)
      if (name != "") names[++n] = name
    }
    END {
      for (i = 1; i <= n; i++) {
        printf "%s%s", (i > 1 ? "," : ""), names[i]
      }
    }
  ' "${SERVER_CONF}"
}

peers_from_deskflow_conf() {
  grep -E '^\[screen_' "${CONF}" 2>/dev/null \
    | sed -E 's/^\[screen_([^]]+)\]$/\1/' \
    | paste -sd, -
}

current_peers() {
  awk '
    /^\[coordination\]/ { in_coord = 1; next }
    /^\[/ && in_coord { exit }
    in_coord && /^peers=/ {
      sub(/^peers=/, "")
      print
      exit
    }
  ' "${CONF}"
}

existing="$(current_peers || true)"
if [[ -n "${existing}" ]]; then
  echo "sync-debug-settings: coordination/peers already set (${existing})"
  exit 0
fi

peers="$(peers_from_server_conf 2>/dev/null || true)"
if [[ -z "${peers}" ]]; then
  peers="$(peers_from_deskflow_conf 2>/dev/null || true)"
fi

if [[ -z "${peers}" ]]; then
  echo "sync-debug-settings: no screen names found in ${SERVER_CONF} or ${CONF}"
  echo "  Configure server screens in the installed app first, or add auto-switch peers manually."
  exit 0
fi

if grep -q '^\[coordination\]' "${CONF}"; then
  sed -i '' "/^\[coordination\]/a\\
enabled=true\\
peers=${peers}
" "${CONF}"
else
  cat >>"${CONF}" <<EOF

[coordination]
enabled=true
peers=${peers}
EOF
fi

echo "sync-debug-settings: set coordination/peers=${peers} in ${CONF}"
