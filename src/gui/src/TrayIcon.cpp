/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2021 Symless Ltd.
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

#include "TrayIcon.h"
#include "gui/constants.h"

TrayIcon::TrayIcon() {
  m_setIcon = [this](const QIcon &icon) {
    m_init = [this, icon]() { this->setIcon(icon); };
  };
}

void TrayIcon::createLoop() const {
  // HACK: apparently this is needed to create a dbus connection, and the hide
  // is needed to make use of the object so the dbus connection doesn't get
  // optimized away by the compiler.
  // TODO: we should verify that this hack actually works.
  QSystemTrayIcon trayIcon;
  trayIcon.hide();

  if (QSystemTrayIcon::isSystemTrayAvailable()) {
    m_pTrayIcon->show();
  } else {
    QTimer::singleShot(2500, this, &TrayIcon::createLoop);
  }
}

void TrayIcon::create(std::vector<QAction *> const &actions) {
  m_pTrayIconMenu = std::make_unique<QMenu>();

  for (auto action : actions) {
    if (action) {
      m_pTrayIconMenu->addAction(action);
    } else {
      m_pTrayIconMenu->addSeparator();
    }
  }

  m_pTrayIcon = std::make_unique<QSystemTrayIcon>();

  connect(
      m_pTrayIcon.get(), &QSystemTrayIcon::activated, this,
      &TrayIcon::activated);

  m_pTrayIcon->setContextMenu(m_pTrayIconMenu.get());
  m_pTrayIcon->setToolTip(kAppName);
  m_setIcon = [this](const QIcon &icon) { m_pTrayIcon->setIcon(icon); };

  createLoop();

  if (m_init) {
    m_init();
    m_init = std::function<void()>();
  }
}
