/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
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
