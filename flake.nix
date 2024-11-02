{
  description = "Nix Flake for Deskflow";
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs?ref=nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    flake-compat = {
      url = "github:edolstra/flake-compat";
      flake = false;
    };
  };
  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
      ...
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.outputs.legacyPackages.${system};
      in
      {
        packages = {
          deskflow = pkgs.qt6Packages.callPackage ./deskflow.nix { };
          default = self.packages.${system}.deskflow;
        };

        devShells.default = pkgs.mkShell { buildInputs = [ self.packages.${system}.default ]; };
      }
    )
    // {
      overlays.default = final: prev: { inherit (self.packages.${final.system}) deskflow; };
    };
}
