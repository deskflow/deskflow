# SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
# SPDX-License-Identifier: MIT
#
# Linux override — reuses the nixpkgs Deskflow package with src swapped to
# the flake's source tree. Inherits nixpkgs' fixup logic (XKB substitution,
# os-release handling, Qt wrapping, postInstall docs, checkPhase).
{ nixpkgsPkg, self }:
nixpkgsPkg.overrideAttrs (old: {
  # Build from the flake's source tree instead of the nixpkgs-pinned tag.
  # Inherits all of nixpkgs' buildInputs, postPatch, cmakeFlags, checkPhase,
  # postInstall, and qtWrapperArgs.
  src = self;
  # Override nixpkgs' version (which tracks its pinned tag) with the flake's
  # git revision — the source may be ahead of nixpkgs. CMake also reads git
  # tags for the binary's internal version.
  version = "unstable-${self.shortRev or "dirty"}";
})
