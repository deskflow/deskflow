/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2021 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/Enums.h"

#include <QString>
#include <QWidget>

class QWidget;

namespace deskflow::gui {

class ClientConnection : public QObject
{
  Q_OBJECT

public:
  explicit ClientConnection(QWidget *parent) : m_pParent(parent)
  {
    // do nothing
  }

  void handleLogLine(const QString &line);

Q_SIGNALS:
  /**
   * @brief requestShowError, This signal is emitted when the client
   * connection would like the owning process to report an error message
   * @param error the type of error being reported
   * @param address of the host
   */
  void requestShowError(deskflow::client::ErrorType error, const QString &address);

private:
  void showMessage(const QString &logLine);

  QWidget *m_pParent;
  bool m_supressMessage = false;
};

} // namespace deskflow::gui
