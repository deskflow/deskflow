#!/usr/bin/env sh

SUDO=$(which sudo > /dev/null 2>&1 && echo "sudo" || echo "")

# Packages for Unix-like BSD-derived.
BSD_PACKAGES=$(cat <<EOF
cmake
ninja
gmake
gcc10
openssl
glib
gdk-pixbuf2
libX11
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
    FreeBSD*)   install_freebsd ;;
    OpenBSD*)   install_openbsd ;;
    NetBSD*)    install_netbsd ;;
    DragonFly*) install_dragonfly ;;
    *)          hint_other $uname_out ;;
  esac
  
  # TODO: remove these debugging lines
  find /usr/local/lib -name libX*
  find /usr/local/include -name X*
}

install_freebsd() {
  run_cmd pkg install -y $BSD_PACKAGES
}

install_openbsd() {
  run_cmd pkg_add $BSD_PACKAGES
}

install_netbsd() {
  run_cmd pkgin install $BSD_PACKAGES
}

install_dragonfly() {
  run_cmd pkg install -y $BSD_PACKAGES
}

hint_other() {
  # TODO: Port the .py script to shell script
  # to make the deps installation lighter on Unix-like.
  echo "For $1 please use: ./scripts/install_deps.py"
}

run_cmd() {
  cmd="${SUDO:+$SUDO }$@"
  echo "Running command: $cmd"
  $cmd
}

install_deps
