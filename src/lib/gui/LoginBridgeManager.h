/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QString>
#include <QStringList>

namespace deskflow::gui {

/**
 * @brief Manages the macOS login-window input bridge (deskflow-vhid-bridge).
 *
 * The bridge replays the elected server's input stream through a Karabiner
 * DriverKit virtual HID device so the cluster can drive this machine at the
 * login window, where CGEventPost is blocked. It runs as a LoginWindow-session
 * launchd agent owned by root, so enabling/disabling prompts for admin rights.
 *
 * macOS only; the implementation file is excluded from other platforms.
 */
class LoginBridgeManager
{
public:
  /// True when the Karabiner VirtualHIDDevice daemon app is installed.
  static bool driverInstalled();

  /// True when the Karabiner VirtualHIDDevice daemon process is running.
  static bool daemonRunning();

  /// True when the bridge LoginWindow launchd agent plist is installed.
  static bool agentInstalled();

  /// Release page for the Karabiner DriverKit VirtualHIDDevice package.
  static QString driverDownloadUrl();

  /// One-line human status for the settings dialog.
  static QString statusText();

  /**
   * @brief Install or remove the bridge LoginWindow agent (admin prompt).
   * @param enabled install the agent when true, remove it when false.
   * @param scale counts-per-point sensitivity passed to the bridge.
   * @param error receives a human-readable reason on failure.
   * @return true when the system state matches @p enabled afterwards.
   */
  static bool apply(bool enabled, double scale, QString *error);

private:
  static QString bridgePath();
  static QString agentPlistPath();
  static QString plistContent(double scale);
  /// Server-candidate hosts derived from coordination/peers, excluding self.
  static QStringList serverCandidates();
};

} // namespace deskflow::gui
