#!/bin/bash

sudo=$(command -v sudo &> /dev/null && echo "sudo" || echo "")

# Packages for Unix-like BSD-derived and Solaris.
unix_like_packages=$(cat <<EOF
cmake
ninja
gmake
gcc10
openssl
glib
gdk-pixbuf2
libXtst
libnotify
libxkbfile
qt6-base
qt6-tools
gtk3
googletest
pugixml
EOF
)

install_deps() {
  uname_out="$(uname -s)"
  case "${uname_out}" in
    FreeBSD*) install_freebsd ;;
    *)        echo "Not supported: ${uname_out}" ;;
  esac
}

install_freebsd() {
  run_cmd pkg install -y $unix_like_packages
}

run_cmd() {
  cmd = $sudo $@
  echo "Running command: $cmd"
  $cmd
}

install_deps
