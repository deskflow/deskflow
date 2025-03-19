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
  const auto session = getenv("XDG_SESSION_TYPE");
  return session != nullptr && std::string(session) == "wayland";
}

} // namespace deskflow::platform
