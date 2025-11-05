/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QFileInfo>
#include <QSysInfo>

namespace deskflow::platform {

inline bool isWayland()
{
  return qEnvironmentVariable("XDG_SESSION_TYPE") == QStringLiteral("wayland");
}

/**
 * @brief isWindows
 * @return Returns true if we are running on windows
 */
inline bool isWindows()
{
  return QSysInfo::productType() == QStringLiteral("windows");
}

/**
 * @brief isMac
 * @return Returns true if we are running on mac os
 */
inline bool isMac()
{
  return QSysInfo::productType() == QStringLiteral("macos");
}

/**
 * @brief isFlatpak
 * @return Returns true if we are running as flatpak
 */
inline bool isFlatpak()
{
  return QFileInfo::exists(QStringLiteral("/.flatpak-info"));
}

/**
 * @brief isSnap
 * @return Returns true if we are running as a snap
 */
inline bool isSnap()
{
  return qEnvironmentVariableIsSet("SNAP");
}

/**
 * @brief isSandboxed
 * @return Returns true if we are running from in a known sandbox.
 */
inline bool isSandboxed()
{
  return isFlatpak() || isSnap();
}

} // namespace deskflow::platform
