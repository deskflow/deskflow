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
