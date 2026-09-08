# SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
# SPDX-License-Identifier: MIT
#
# Deskflow derivation — builds from source using CMake/Qt6.
# Imported by the root flake.nix.
{
  lib,
  pkgs,
  self,
}:
let
  isLinux = pkgs.stdenv.hostPlatform.isLinux;
  isDarwin = pkgs.stdenv.hostPlatform.isDarwin;
in
pkgs.stdenv.mkDerivation {
  pname = "deskflow";
  # Derive from the flake's git revision — CMake also reads git tags for the
  # binary's internal version, so this stays honest with the actual source.
  version = "unstable-${self.shortRev or "dirty"}";

  src = self;

  nativeBuildInputs = [
    pkgs.cmake
    pkgs.ninja
    pkgs.pkg-config
    pkgs.qt6.wrapQtAppsHook
    pkgs.doxygen
  ];

  buildInputs = [
    pkgs.qt6.qtbase
    pkgs.qt6.qttools
    pkgs.qt6.qttranslations
    pkgs.openssl
    pkgs.pugixml
    pkgs.python3
  ]
  ++ lib.optionals isLinux [
    pkgs.gtest
    pkgs.libei
    pkgs.libportal
    pkgs.libx11
    pkgs.libxkbfile
    pkgs.libxinerama
    pkgs.libxi
    pkgs.libxrandr
    pkgs.libxtst
    pkgs.libxkbcommon
    pkgs.gdk-pixbuf
    pkgs.libnotify
    pkgs.qt6.qtwayland
    pkgs.qt6.qtdeclarative
    pkgs.wayland
    pkgs.wayland-protocols
    pkgs.libsysprof-capture
    pkgs.lerc
  ];

  cmakeFlags = [
    "-DCMAKE_SKIP_RPATH=ON"
    "-DSKIP_BUILD_TESTS=ON"
    "-DBUILD_INSTALLER=OFF"
  ];

  preConfigure = lib.optionalString isDarwin ''
    # CMake unconditionally checks for macdeployqt on macOS (even with
    # BUILD_INSTALLER=OFF). The binary is in qtbase's bin directory.
    export PATH="${pkgs.qt6.qtbase}/bin:$PATH"
  '';

  postPatch = ''
    # Qt translation files are in a separate qttranslations package in nixpkgs.
    substituteInPlace translations/CMakeLists.txt \
      --replace-fail 'PATHS ''${QT_ROOT_DIR} PATH_SUFFIXES "translations" "share/qt/translations"' 'PATHS "${pkgs.qt6.qttranslations}/translations"'
    # nixpkgs Qt6 disables the cxx17_filesystem feature, so
    # QFile::filesystemFileName() is unavailable. Use the equivalent
    # conversion via QString::toStdString().
    substituteInPlace src/lib/net/SecureUtils.cpp \
      --replace-fail "file.filesystemFileName()" "std::filesystem::path(file.fileName().toStdString())"
  ''
  + lib.optionalString isLinux ''
    substituteInPlace src/lib/deskflow/unix/AppUtilUnix.cpp \
      --replace-fail "/usr/share/X11/xkb/rules/evdev.xml" "${pkgs.xkeyboard_config}/share/X11/xkb/rules/evdev.xml"
  ''
  + lib.optionalString isDarwin ''
    # The project requests static OpenSSL on macOS to avoid Apple's
    # system OpenSSL. With nixpkgs OpenSSL (shared libs only), this is
    # unnecessary — we're not using the system OpenSSL.
    substituteInPlace src/lib/net/CMakeLists.txt \
      --replace-fail "set(OPENSSL_USE_STATIC_LIBS TRUE)" ""
  '';

  qtWrapperArgs = lib.optionals isLinux [
    "--set QT_QPA_PLATFORM_PLUGIN_PATH ${pkgs.qt6.qtwayland}/${pkgs.qt6.qtbase.qtPluginPrefix}/platforms"
  ];

  strictDeps = true;

  meta = {
    homepage = "https://github.com/deskflow/deskflow";
    description = "Share one mouse and keyboard between multiple computers on Windows, macOS and Linux";
    mainProgram = "deskflow";
    license = with lib.licenses; [
      gpl2Plus
      openssl
      mit
    ];
    platforms = lib.platforms.unix;
  };
}
