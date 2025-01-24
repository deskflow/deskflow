/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "gui/config/IAppConfig.h"

#include "gui/messages.h"

#include <QObject>
#include <QString>
#include <QWidget>
#include <memory>

class QWidget;

namespace deskflow::gui {

class ClientConnection : public QObject
{
  Q_OBJECT

public:
  struct Deps
  {
    virtual ~Deps() = default;
    virtual void showError(QWidget *parent, messages::ClientError error, const QString &address) const;
  };

  explicit ClientConnection(
      QWidget *parent, IAppConfig &appConfig, std::shared_ptr<Deps> deps = std::make_shared<Deps>()
  )
      : m_pParent(parent),
        m_appConfig(appConfig),
        m_deps(deps)
  {
  }

  void handleLogLine(const QString &line);
  void setShowMessage()
  {
    m_showMessage = true;
  }

signals:
  void messageShowing();

private:
  void showMessage(const QString &logLine);

  QWidget *m_pParent;
  IAppConfig &m_appConfig;
  std::shared_ptr<Deps> m_deps;
  bool m_showMessage = true;
};

} // namespace deskflow::gui
