{
  lib,
  pkgs,
  python3,
  stdenv,
  cmake,
  withLibei ? true,
  avahi,
  curl,
  libICE,
  libSM,
  libX11,
  libXdmcp,
  libXext,
  libXinerama,
  libXrandr,
  libXtst,
  libxkbfile,
  openssl,
  pkg-config,
  qt6,
  wrapGAppsHook3,
  pugixml,
  libnotify,
  gtest,
  lerc,
  cli11,
  tomlplusplus,
}:
let
  isDarwin = lib.hasSuffix "-darwin" pkgs.system;
in
stdenv.mkDerivation rec {
  pname = "deskflow";
  version = "unstable";

  src = ./.;

  nativeBuildInputs = [
    pkg-config
    cmake
    wrapGAppsHook3
    qt6.wrapQtAppsHook
    qt6.qttools
  ];
  buildInputs =
    [
      curl
      qt6.qtbase
      (avahi.override { withLibdnssdCompat = true; })
      openssl
      libX11
      libXext
      libXtst
      libXinerama
      libXrandr
      libXdmcp
      libxkbfile
      libICE
      libSM
      pugixml
      python3
      libnotify
      gtest
      lerc
      cli11
      tomlplusplus
    ]
    ++ lib.optionals (withLibei && !isDarwin) (
      with pkgs;
      [
        libei
        libportal
        qt6.qtwayland
      ]
    );

  cmakeFlags = [
    "-DDESKFLOW_REVISION=${version}"
    "-DCMAKE_SKIP_BUILD_RPATH=ON"
  ];

  dontWrapGApps = true;
  preFixup = ''
    qtWrapperArgs+=(
      "''${gappsWrapperArgs[@]}"
        --prefix PATH : "${lib.makeBinPath [ openssl ]}"
    )
  '';

  postFixup = ''
    substituteInPlace $out/share/applications/deskflow.desktop \
      --replace "Exec=deskflow" "Exec=$out/bin/deskflow"
  '';

  meta = with lib; {
    description = "Mouse and keyboard sharing utility";
    longDescription = ''
      Deskflow lets you share one mouse and keyboard between multiple computers on Windows, macOS and Linux.
      It's like a software KVM (but without video). 
    '';
    homepage = "https://github.com/deskflow/deskflow";
    license = licenses.gpl2;
    mainProgram = "deskflow";
    maintainers = with maintainers; [ shymega ];
    platforms = platforms.linux ++ platforms.darwin;
  };
}
