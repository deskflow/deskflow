/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "WaylandWarnings.h"

#include "messages.h"

using namespace deskflow::platform;

namespace deskflow::gui::core {

//
// WaylandWarnings::Deps
//

void WaylandWarnings::Deps::showWaylandLibraryError(QWidget *parent)
{
  messages::showWaylandLibraryError(parent);
}

//
// WaylandWarnings
//

void WaylandWarnings::showOnce(
    QWidget *parent, CoreProcess::Mode mode, bool hasEi, bool hasPortal, bool hasPortalInputCapture
)
{

  const auto portalIcProblem = !hasPortalInputCapture && mode == CoreProcess::Mode::Server;

  if (!hasEi || !hasPortal || portalIcProblem) {
    if (!m_errorShown) {
      m_errorShown = true;
      m_pDeps->showWaylandLibraryError(parent);
    } else {
      qWarning("missing required wayland lib(s) or feature");
    }
    return;
  }
}

} // namespace deskflow::gui::core
