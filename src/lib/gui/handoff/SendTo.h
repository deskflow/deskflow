/*
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#pragma once
#include "Transfer.h"
#include <QObject>

class QWidget;
class ServerConfig;
class QMenu;

namespace deskflow::handoff {
class SendTo : public QObject
{
public:
  SendTo(QWidget *parent, const ServerConfig &config, QMenu *fileMenu, QMenu *trayMenu);
  ~SendTo() override;

private:
  void chooseComputer(Request request);
  QWidget *m_window;
  const ServerConfig &m_config;
};
} // namespace deskflow::handoff
