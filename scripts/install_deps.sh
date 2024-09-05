#!/usr/bin/env sh

SUDO=$(which sudo > /dev/null 2>&1 && echo "sudo" || echo "")

install_deps() {
  uname_out="$(uname -s)"
  case "${uname_out}" in
    FreeBSD*)   install_freebsd ;;
    OpenBSD*)   install_openbsd ;;
    NetBSD*)    install_netbsd ;;
    DragonFly*) install_dragonfly ;;
    SunOS*)     install_solaris ;;
    *)          hint_other $uname_out ;;
  esac
}

install_freebsd() {
  run_cmd pkg install -y \
    cmake \
    ninja \
    gmake \
    gcc10 \
    openssl \
    glib \
    gdk-pixbuf2 \
    libX11 \
    libXtst \
    libnotify \
    libxkbfile \
    qt6-base \
    qt6-tools \
    gtk3 \
    googletest \
    pugixml
}

install_openbsd() {
  # Patches welcome!
  # pkg_add error:
  #   Can't find libX11
  #   Can't find libXtst
  echo "Sorry, OpenBSD is not supported yet."
}

install_netbsd() {
  # Patches welcome!
  # pkg_add error:
  #  pkg_add: no pkg found for 'libX11', sorry.
  #  pkg_add: no pkg found for 'libXtst', sorry.
  echo "Sorry, NetBSD is not supported yet."
}

install_dragonfly() {
  # Patches welcome!
  # The C++ version on DragonFly BSD seems to be too old.
  echo "Sorry, DragonFly BSD is not supported yet."
}

install_solaris() {
  # Patches welcome!
  echo "Sorry, Solaris is not supported yet."
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
