/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "common/Enums.h"
#include "gui/RecentServerHistory.h"

#include <QToolButton>

class RecentServersButton : public QToolButton
{
  Q_OBJECT

public:
  explicit RecentServersButton(QWidget *parent = nullptr);
  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;
  void loadHistory(const QString &json);
  QString history() const;
  void setConnectionAllowed(bool allowed, const QString &reason = {});
  void beginAttempt(const QString &address, int port, bool tlsAndPeerChecksEnabled);
  void cancelAttempt();
  void connectionStateChanged(deskflow::core::ConnectionState state);
  void secureSocket(bool enabled);

Q_SIGNALS:
  void reconnectRequested(const QString &address, int port);
  void historyChanged(const QString &json);

protected:
  void changeEvent(QEvent *event) override;

private:
  void refresh();
  void publishChange(bool changed);
  void requestConnection(const deskflow::gui::RecentServerHistory::Entry &entry);

  deskflow::gui::RecentServerHistory m_history;
  bool m_connectionAllowed = false;
  QString m_unavailableReason;
};
