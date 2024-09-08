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

#include <QWidget>
#include <memory>

#include "CoreProcess.h"
#include "platform/wayland.h"

namespace synergy::gui::core {

class WaylandWarnings {
public:
  struct Deps {
    virtual ~Deps() = default;
    virtual void showWaylandExperimental(QWidget *parent);
    virtual void showNoEiSupport(QWidget *parent);
    virtual void showNoPortalInputCapture(QWidget *parent);
  };

  explicit WaylandWarnings(
      std::shared_ptr<Deps> deps = std::make_shared<Deps>())
      : m_pDeps(deps) {}

  void showOnce(
      QWidget *parent, CoreProcess::Mode mode, bool hasEi = platform::kHasEi,
      bool hasPortalInputCapture = platform::kHasPortalInputCapture);

private:
  bool m_shown{false};
  std::shared_ptr<Deps> m_pDeps;
};

} // namespace synergy::gui::core
