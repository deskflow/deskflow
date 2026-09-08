# SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
# SPDX-License-Identifier: MIT
#
# Per-system outputs for the Deskflow flake.
# Imported by the root flake.nix.
#
# Hybrid approach:
# - Linux: overrideAttrs on the nixpkgs package (inherits nixpkgs fixup + testing)
# - Darwin: from-source build (nixpkgs doesn't package Deskflow for macOS)
{
  lib,
  pkgs,
  self,
  nixpkgs,
  system,
}:
let
  isLinux = pkgs.stdenv.hostPlatform.isLinux;
  isDarwin = pkgs.stdenv.hostPlatform.isDarwin;

  # From-source build — used on Darwin (nixpkgs doesn't cover macOS) and
  # exposed as #source on all platforms.
  sourceDeskflow = import ./deskflow.nix { inherit lib pkgs self; };

  # nixpkgs-packaged version — Linux only (nixpkgs meta.platforms = linux).
  nixpkgsPkg = nixpkgs.legacyPackages.${system}.deskflow;

  # Linux: override nixpkgs package's src to build from the flake's source.
  # Inherits nixpkgs' fixup logic (XKB substitution, os-release handling,
  # Qt wrapping, postInstall docs, checkPhase).
  overrideDeskflow = import ./override.nix { inherit nixpkgsPkg self; };

  # Hybrid: nixpkgs override on Linux, from-source on Darwin.
  deskflow =
    if isLinux then overrideDeskflow else sourceDeskflow;

  # On macOS, the binary is inside a .app bundle.
  binPath =
    if isDarwin then "${deskflow}/Deskflow.app/Contents/MacOS/deskflow" else "${deskflow}/bin/deskflow";
in
{
  packages = {
    inherit deskflow;
    source = sourceDeskflow;
    default = deskflow;
  }
  // lib.optionalAttrs isLinux {
    nixpkgs = nixpkgsPkg;
  };

  apps = {
    deskflow = {
      type = "app";
      program = binPath;
    };
    default = {
      type = "app";
      program = binPath;
    };
  }
  // lib.optionalAttrs isLinux {
    nixpkgs = {
      type = "app";
      program = "${nixpkgsPkg}/bin/deskflow";
    };
  };

  devShells.default = pkgs.mkShell {
    packages = sourceDeskflow.buildInputs ++ sourceDeskflow.nativeBuildInputs;
  };
}
