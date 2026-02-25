/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "WaylandWarnings.h"

#include "Messages.h"
#include "common/Settings.h"

namespace deskflow::gui::core {

void WaylandWarnings::showOnce(QWidget *parent)
{
  const auto mode = Settings::value(Settings::Core::CoreMode).value<Settings::CoreMode>();

  if (!m_hasEi || !m_hasPortal || mode == Settings::CoreMode::Server) {
    if (!m_errorShown) {
      m_errorShown = true;
      messages::showWaylandLibraryError(parent);
    } else {
      qWarning("missing required wayland lib(s) or feature");
    }
    return;
  }
}

} // namespace deskflow::gui::core
