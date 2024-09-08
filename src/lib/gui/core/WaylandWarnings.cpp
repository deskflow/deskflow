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
    QWidget *parent, CoreProcess::Mode mode, bool hasEi,
    bool hasPortalInputCapture) {
  if (m_shown) {
    qDebug("wayland warnings already shown");
    return;
  }

  m_shown = true;

  if (!hasEi) {
    qWarning("libei is missing, required for wayland support mode");
    m_pDeps->showNoEiSupport(parent);
    return;
  }

  if (!hasPortalInputCapture && mode == CoreProcess::Mode::Server) {
    qWarning("libportal is missing input capture, required for server mode");
    m_pDeps->showNoPortalInputCapture(parent);
    return;
  }

  m_pDeps->showWaylandExperimental(parent);
}

} // namespace synergy::gui::core
