/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QMap>
#include <QStatusBar>

#include "common/Enums.h"

class QPushButton;
class QLabel;

using ProcessState = deskflow::core::ProcessState;
using ConnectionState = deskflow::core::ConnectionState;

class StatusBar : public QStatusBar
{
  Q_OBJECT
public:
  explicit StatusBar(QWidget *parent = nullptr);
  void setStatus(ConnectionState connectionState, ProcessState processState, bool isServer);
  void setServerClients(const QStringList &clients);
  void setSecurityIconVisible(bool visible);
  void setConnectionInterval(int newInterval);
  bool securityIconVisible() const;
  void updateSecurityInfo(bool encrypted);
  void setSecurityIcon(bool encrypted);
  void setSecurityLevel(const QString &securityLevel);
  void setBtnFingerprintVisible(bool visible);
  void updateFound(const QString &version);
  void showClipboardSending(qint64 bytes, const QString &peer);
  void showClipboardSent(const QString &peer);
  void showClipboardReceiving(qint64 bytes, const QString &peer);
  void showClipboardReceived(const QString &peer);
  void showClipboardOverLimit(qint64 bytes, qint64 limit);

Q_SIGNALS:
  void requestShowMyFingerprints();
  void requestUpdateVersion();

protected:
  void changeEvent(QEvent *e) override;

private:
  void updateText();
  void updateTimerLabel();
  void updateClipboardIcons();
  static void showClipboardIconAfterDelay(QLabel *label, QTimer *delay, bool active);
  static QString clipboardPeerName(const QString &peer);

  static constexpr int kClipboardNoticeTimeoutMs = 30000;
  static constexpr int kClipboardIconDelayMs = 500;

  QPushButton *m_btnFingerprint = nullptr;
  QLabel *m_lblSecurityIcon = nullptr;
  QLabel *m_lblStatus = nullptr;
  QLabel *m_lblClipboardSending = nullptr;
  QLabel *m_lblClipboardReceiving = nullptr;
  QLabel *m_lblClipboardOverLimit = nullptr;
  QPushButton *m_btnUpdate = nullptr;
  bool m_encrypted = false;
  QString m_securityLevel;
  int m_connectionInterval = -1;
  QTimer *m_retryTimer = nullptr;
  QTimer *m_clipboardSendingTimer = nullptr;
  QTimer *m_clipboardReceivingTimer = nullptr;
  QTimer *m_clipboardOverLimitTimer = nullptr;
  QMap<QString, qint64> m_clipboardSendingTo;
  QMap<QString, qint64> m_clipboardReceivingFrom;
};
