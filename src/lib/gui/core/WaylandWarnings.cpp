/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "WaylandWarnings.h"

#include "Messages.h"
#include "common/Settings.h"

using namespace deskflow::platform;

namespace deskflow::gui::core {

void WaylandWarnings::showOnce(QWidget *parent, bool hasEi, bool hasPortal, bool hasPortalInputCapture)
{
  const auto mode = Settings::value(Settings::Core::CoreMode).value<Settings::CoreMode>();
  const bool portalIcProblem = !hasPortalInputCapture && mode == Settings::CoreMode::Server;

  if (!hasEi || !hasPortal || portalIcProblem) {
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
