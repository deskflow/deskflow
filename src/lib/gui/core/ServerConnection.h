/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QString>
#include <QStringList>

#include "gui/Messages.h"
#include "gui/config/IServerConfig.h"

namespace deskflow::gui {

class ServerConnection : public QObject
{
  Q_OBJECT
  using IServerConfig = deskflow::gui::IServerConfig;

public:
  struct Deps
  {
    virtual ~Deps() = default;
    virtual messages::NewClientPromptResult
    showNewClientPrompt(QWidget *parent, const QString &clientName, bool serverRequiresPeerAuth = false) const;
  };

  explicit ServerConnection(
      QWidget *parent, IServerConfig &serverConfig, std::shared_ptr<Deps> deps = std::make_shared<Deps>()
  );
  void handleLogLine(const QString &logLine);

Q_SIGNALS:
  void messageShowing();
  void configureClient(const QString &clientName);

private:
  void handleNewClient(const QString &clientName);

  QWidget *m_pParent;
  IServerConfig &m_serverConfig;
  std::shared_ptr<Deps> m_pDeps;
  QStringList m_receivedClients;
  bool m_messageShowing = false;
};

} // namespace deskflow::gui
