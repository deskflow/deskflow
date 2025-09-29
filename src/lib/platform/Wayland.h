/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QFileInfo>

namespace deskflow::platform {

#if WINAPI_LIBEI
const auto kHasEi = true;
#else
const auto kHasEi = false;
#endif

#if WINAPI_LIBPORTAL
const auto kHasPortal = true;
#else
const auto kHasPortal = false;
#endif

#if HAVE_LIBPORTAL_INPUTCAPTURE
const auto kHasPortalInputCapture = true;
#else
const auto kHasPortalInputCapture = false;
#endif

inline bool isWayland()
{
  return qEnvironmentVariable("XDG_SESSION_TYPE") == QStringLiteral("wayland");
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
