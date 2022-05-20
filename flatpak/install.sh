#!/bin/sh
flatpak-builder --user --install --force-clean build com.symless.Synergy.yml
flatpak run com.symless.Synergy