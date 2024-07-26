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

void TrayIcon::tryCreate() const {
  QSystemTrayIcon trayIcon; // by creating a new tray icon, we actually make the
                            // DBus implementation refresh the connection (DBus)
  trayIcon.hide();          // we ony hide it in order for the compiler to not
                            // optimise-out the object (make some use of it)
  if (QSystemTrayIcon::isSystemTrayAvailable()) { // this ends up calling the
                                                  // underlying DBus connection
                                                  // (on DBus)
    m_pTrayIcon->show();
    m_connector(
        m_pTrayIcon.get(),
        SIGNAL(activated(QSystemTrayIcon::ActivationReason)));
  } else {
    QTimer::singleShot(2500, this, &TrayIcon::tryCreate);
  }
}
