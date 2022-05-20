#!/bin/sh
flatpak build-export export build
flatpak build-bundle export synergy.flatpak com.symless.Synergy master --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo