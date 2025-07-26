/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <string>

namespace deskflow::platform {

#if WINAPI_LIBEI
const auto kHasEi = true;
#else
const auto kHasEi = false;
#endif

#ifndef __APPLE__
const auto kHasPortal = true; // Using QDbus for portal access
#else
const auto kHasPortal = false;
#endif

const auto kHasPortalClipboard = false;
const auto kHasPortalInputCapture = false;

inline bool isWayland()
{
  const auto session = std::getenv("XDG_SESSION_TYPE");
  return session != nullptr && std::string(session) == "wayland";
}

} // namespace deskflow::platform
