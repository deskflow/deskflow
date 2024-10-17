/*
 * Deskflow -- mouse and keyboard sharing utility
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
