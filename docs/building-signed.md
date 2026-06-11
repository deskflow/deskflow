# Building signed fleet binaries

How to produce installable, *identity-signed* builds of this fork for every
machine in a personal KVM fleet. Signing with a real certificate (not ad-hoc)
matters on macOS because TCC anchors trust to the signing identity: a stable
identity means Accessibility / Input Monitoring grants survive rebuilds and
reinstalls instead of re-prompting every time the binary changes.

## Certificates: what you need (and what you don't)

| Platform | For your own machines | For public distribution |
|---|---|---|
| macOS | A free **Apple Development** certificate (Xcode → Settings → Accounts → Manage Certificates → "+" → Apple Development). No paid program needed. | **Developer ID Application** cert + notarization (paid Apple Developer Program). |
| Windows | Nothing — unsigned binaries run fine locally; SmartScreen only screens downloaded files. | An Authenticode certificate (separate purchase; Apple certs are useless here). |
| Linux | Nothing. | Distro-dependent (repo signing), out of scope. |

List available macOS identities:

```sh
security find-identity -v -p codesigning
```

## macOS (per-arch: build natively on each Mac)

The repo's `APPLE_CODESIGN_DEV` option signs the bundle with hardened runtime
and the dev entitlements after every build, and (this fork) the deployed DMG
app as well:

```sh
export IDENTITY="Apple Development: Your Name (TEAMID)"

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qt)" \
  -DAPPLE_CODESIGN_DEV="$IDENTITY"
ninja -C build

# Verify
codesign --verify --deep --strict build/bin/Deskflow.app && echo OK

# Self-contained installable DMG (bundles Qt via macdeployqt, signs with
# the same identity):
(cd build && cpack -G DragNDrop)   # -> build/Deskflow-<ver>-macos-<arch>.dmg
```

Architecture note: a Mac builds for its own architecture. Build the Intel
DMG on the Intel Mac and the Apple Silicon DMG on the arm64 Mac (or pass
`-DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"` for a universal build if every
dependency — brew Qt is per-arch — is available universal, which Homebrew's
usually is not; native-per-machine is the simple, reliable path).

### Using the same identity on a second Mac

The certificate's private key lives in the keychain of the Mac that created
it. To sign on another Mac with the same identity:

1. On the source Mac: Keychain Access → My Certificates → right-click the
   "Apple Development: …" item → Export as `.p12` (set a passphrase).
2. Copy to the target Mac, double-click to import into the login keychain.
3. The same `-DAPPLE_CODESIGN_DEV` value now works there.

(Alternatively sign in to Xcode on the second Mac with the same Apple ID and
mint a second Apple Development cert — different serial, same team, equally
fine for TCC.)

### Known x86_64 quirk (fixed in this fork)

The arm64 linker ad-hoc signs everything it produces; the x86_64 linker does
not. Upstream's codesign step signed only the bundle and failed on Intel with
"code object is not signed at all" for nested executables.
`cmake/MacCodesign.cmake` here signs the nested binaries first.

## Windows

Prereqs on the Windows machine: Visual Studio Build Tools (C++ workload),
CMake 3.24+, Qt 6 (the official installer's MSVC kit), and optionally
[WiX v4+](https://wixtoolset.org) for an `.msi` (a `.7z` is produced either
way).

```bat
cmake -B build -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_PREFIX_PATH=C:\Qt\6.7.0\msvc2019_64
cmake --build build --config Release
cd build && cpack            :: -> .7z always, .msi when WiX is installed
```

Signing (only if you own an Authenticode cert):

```bat
signtool sign /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 ^
  /f yourcert.pfx /p <passphrase> build\bin\*.exe
```

For a personal fleet, skip it — Windows does not gate locally built/installed
binaries the way macOS TCC does.

## Linux

```sh
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build && (cd build && cpack)   # rpm/deb per deploy/linux/deploy.cmake
```

## Mouser (companion app)

Mouser's macOS build script takes the same identity via env:

```sh
cd ../Mouser
MOUSER_SIGN_IDENTITY="$IDENTITY" ./build_macos_app.sh   # -> dist/Mouser.app
```

Unset, it builds unsigned (ad-hoc) — fine functionally, but the TCC-stability
argument above applies to Mouser too (it needs Input Monitoring).
