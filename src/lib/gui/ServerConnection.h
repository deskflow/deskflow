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

#include <QString>
#include <QStringList>

#include "AppConfig.h"
#include "IServerConfig.h"

class ServerConnection : public QObject {
  Q_OBJECT
  using IServerConfig = synergy::gui::IServerConfig;

public:
  explicit ServerConnection(
      QWidget &parent, AppConfig &appConfig, IServerConfig &serverConfig);
  void update(const QString &line);

signals:
  void configureClient(const QString &clientName);

private:
  void addClient(const QString &clientName);
  bool checkMainWindow();

  QWidget &m_parent;
  AppConfig &m_appConfig;
  IServerConfig &m_serverConfig;
  QStringList m_ignoredClients;
};
