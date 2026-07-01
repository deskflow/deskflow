/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QString>

void requestOSXNotificationPermission();
bool isOSXDevelopmentBuild();
bool showOSXNotification(const QString &title, const QString &body);
bool isOSXInterfaceStyleDark();
//! When \p visible is false, remove the app from the Dock (Accessory policy).
//! When true, show in the Dock and bring the app forward (Regular policy).
void macOSSetDockVisible(bool visible);
//! True when the app uses Regular activation policy (visible in the Dock).
bool macOSIsDockVisible();
void forceAppActive();
void macOSNativeHide();

//! Register/unregister this .app as a macOS login item (SMAppService).
/*! Lets Deskflow start itself at login with no external LaunchAgent --
    the app owns its own launch. Returns true when the resulting state
    matches \p enable. No-op/false on macOS < 13. */
bool macSetStartAtLogin(bool enable);

//! True when this .app is registered as a login item (the system is the
//! source of truth, like the login-window bridge).
bool macStartAtLoginEnabled();

/**
 * @brief isOSXAccessibilityGranted Check whether this process is trusted to control
 * the keyboard and mouse (System Settings > Privacy & Security > Accessibility).
 * @param promptUser When true, macOS shows its native prompt and adds the app to the
 * Accessibility list if it is not already there.
 * @return true when accessibility access has been granted.
 */
bool isOSXAccessibilityGranted(bool promptUser);

/**
 * @brief openOSXAccessibilitySettings Open the Accessibility pane of System Settings.
 */
void openOSXAccessibilitySettings();
