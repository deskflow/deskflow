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

#pragma once

#include "AppConfig.h"

#include <QString>
#include <QWidget>

namespace synergy::gui {

class ClientConnection {
public:
  explicit ClientConnection(QWidget &parent, AppConfig &appConfig);
  void update(const QString &line);
  void setCheckConnection(bool checkConnection);

private:
  QString getMessage(const QString &line) const;
  bool checkMainWindow();
  void showMessage(const QString &message) const;

  QWidget &m_parent;
  AppConfig &m_appConfig;
  bool m_checkConnection = false;
};

} // namespace synergy::gui
