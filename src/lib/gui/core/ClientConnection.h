/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/Enums.h"
#include "gui/Messages.h"

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
    virtual void showError(QWidget *parent, deskflow::client::ErrorType error, const QString &address) const;
  };

  explicit ClientConnection(QWidget *parent, std::shared_ptr<Deps> deps = std::make_shared<Deps>())
      : m_pParent(parent),
        m_deps(deps)
  {
    // do nothing
  }

  void handleLogLine(const QString &line);
  void setShowMessage()
  {
    m_showMessage = true;
  }

Q_SIGNALS:
  /**
   * @brief requestShowError, This signal is emitted when the client
   * connection would like the owning process to report an error message
   * @param error the type of error being reported
   * @param address of the host
   */
  void requestShowError(deskflow::client::ErrorType error, const QString &address);
  void messageShowing();

private:
  void showMessage(const QString &logLine);

  QWidget *m_pParent;
  std::shared_ptr<Deps> m_deps;
  bool m_showMessage = true;
};

} // namespace deskflow::gui
