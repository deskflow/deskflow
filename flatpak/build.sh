#!/bin/sh
flatpak-builder build com.symless.Synergy.yml --disable-cache --force-clean
flatpak build-finish build