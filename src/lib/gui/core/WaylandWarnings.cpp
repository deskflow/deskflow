/*
 * synergy -- mouse and keyboard sharing utility
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

#include "WaylandWarnings.h"

#include "messages.h"

using namespace synergy::platform;

namespace synergy::gui::core {

//
// WaylandWarnings::Deps
//

void WaylandWarnings::Deps::showNoEiSupport(QWidget *parent) {
  messages::showNoEiSupport(parent);
}

void WaylandWarnings::Deps::showNoPortalSupport(QWidget *parent) {
  messages::showNoPortalSupport(parent);
}

void WaylandWarnings::Deps::showNoPortalInputCapture(QWidget *parent) {
  messages::showNoPortalInputCapture(parent);
}

void WaylandWarnings::Deps::showWaylandExperimental(QWidget *parent) {
  messages::showWaylandExperimental(parent);
}

//
// WaylandWarnings
//

void WaylandWarnings::showOnce(
    QWidget *parent, CoreProcess::Mode mode, bool hasEi, bool hasPortal,
    bool hasPortalInputCapture) {
  if (!hasEi) {
    qWarning("libei is missing, required for wayland support mode");
    if (!m_failureShown) {
      m_failureShown = true;
      m_pDeps->showNoEiSupport(parent);
    }
    return;
  }

  if (!hasPortal) {
    qWarning("libportal is missing, required for wayland support mode");
    if (!m_failureShown) {
      m_failureShown = true;
      m_pDeps->showNoPortalSupport(parent);
    }
    return;
  }

  if (!hasPortalInputCapture && mode == CoreProcess::Mode::Server) {
    qWarning("libportal is missing input capture, required for server mode");

    if (!m_failureShown) {
      m_failureShown = true;
      m_pDeps->showNoPortalInputCapture(parent);
    }
    return;
  }

  if (!m_successShown) {
    m_successShown = true;
    m_pDeps->showWaylandExperimental(parent);
  }
}

} // namespace synergy::gui::core
