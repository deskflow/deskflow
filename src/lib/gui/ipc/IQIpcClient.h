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

#pragma once

#include <QObject>
#include <QString>

#include "gui/config/ElevateMode.h"

namespace deskflow::gui::ipc {

class IQIpcClient : public QObject
{
  Q_OBJECT
public:
  ~IQIpcClient() override = default;
  virtual void sendHello() const = 0;
  virtual void sendCommand(const QString &command, ElevateMode elevate) const = 0;
  virtual void connectToHost() = 0;
  virtual void disconnectFromHost() = 0;
  virtual bool isConnected() const = 0;

signals:
  void read(const QString &text);
  void serviceReady();
};

} // namespace deskflow::gui::ipc
