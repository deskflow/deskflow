# SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
# SPDX-License-Identifier: MIT
#
# Root flake — thin shim that delegates to deploy/nix/.
# flake.nix must live at the repository root for `nix run github:...` to work;
# the actual implementation is in deploy/nix/.
{
  description = "Deskflow — share one mouse and keyboard between multiple computers";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    # nixpkgs-unstable (26.11) dropped x86_64-darwin support. Pin x86_64-darwin
    # to the 26.05 stable branch, which still supports it and uses a newer SDK
    # (14.x) than the older -darwin branches.
    nixpkgs-darwin-legacy.url = "github:NixOS/nixpkgs/nixpkgs-26.05-darwin";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      nixpkgs-darwin-legacy,
      flake-utils,
    }:
    flake-utils.lib.eachSystem
      [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ]
      (
        system:
        let
          pkgs =
            if system == "x86_64-darwin" then
              nixpkgs-darwin-legacy.legacyPackages.${system}
            else
              nixpkgs.legacyPackages.${system};
        in
        import ./deploy/nix/outputs.nix {
          inherit (pkgs) lib;
          inherit pkgs self nixpkgs system;
        }
      );
}
