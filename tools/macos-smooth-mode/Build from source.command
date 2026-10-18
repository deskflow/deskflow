#!/bin/zsh
set -eu
smooth_dir=${0:A:h}
cd "$smooth_dir"
mkdir -p "Deskflow Smooth Mode.app/Contents/MacOS" "Deskflow Smooth Mode.app/Contents/Resources"
cp resources/Info.plist "Deskflow Smooth Mode.app/Contents/Info.plist"
cp README.md "Deskflow Smooth Mode.app/Contents/Resources/README.txt"
clang -O2 -Wall -Wextra -arch arm64 -arch x86_64 -mmacosx-version-min=13.0 \
  awdl-guard.c -o awdl-guard -lproc
clang -O2 -Wall -Wextra -fobjc-arc -arch arm64 -arch x86_64 -mmacosx-version-min=13.0 \
  SmoothSetup.m -o smooth-setup -framework Foundation -lproc
clang -O2 -Wall -Wextra -fobjc-arc -arch arm64 -arch x86_64 -mmacosx-version-min=13.0 \
  SmoothMenu.m -o "Deskflow Smooth Mode.app/Contents/MacOS/SmoothMenu" -framework AppKit
codesign --force --sign - --identifier local.deskflow.mx-smooth-mode "Deskflow Smooth Mode.app"
./awdl-guard --self-test
./smooth-setup --self-test
print "Build and self-tests passed. No services installed or interfaces changed."
