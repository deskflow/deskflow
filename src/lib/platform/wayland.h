/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
