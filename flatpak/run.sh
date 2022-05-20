#!/bin/sh

# Build application
./build.sh

# replace .desktop
cp com.symless.Synergy.desktop ./build/files/share/applications/com.symless.Synergy.desktop
cp com.symless.Synergy.desktop ./build/export/share/applications/com.symless.Synergy.desktop

# run export.sh which includes bundling
./export.sh