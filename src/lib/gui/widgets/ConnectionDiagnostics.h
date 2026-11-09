// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "common/Enums.h"
#include <QDeadlineTimer>
#include <QFrame>
#include <QJsonArray>

class QLabel;
class QPushButton;
class QTimer;

class ConnectionDiagnostics : public QFrame
{
  Q_OBJECT
public:
  enum class Phase
  {
    Idle,
    Starting,
    Discovering,
    Connecting,
    Verifying,
    Recovering,
    Connected,
    Listening,
    Retrying,
    Stopping,
    NeedsAction,
    Paused
  };
  Q_ENUM(Phase)
  enum class Attention
  {
    None,
    Identity,
    SecurityPolicy,
    Address,
    StartFailed,
    Daemon,
    DuplicateName,
    UnknownClient,
    Version,
    Protocol,
    Recovery
  };
  Q_ENUM(Attention)
  explicit ConnectionDiagnostics(QWidget *parent = nullptr);
  Phase phase() const;
  Attention attention() const
  {
    return m_attention;
  }
  int retrySeconds() const
  {
    return m_retryPending ? qMax(0, int((m_deadline.remainingTime() + 999) / 1000)) : -1;
  }
  QString summary() const;
  QString explanation() const
  {
    return detail();
  }
  QByteArray diagnosticSnapshot() const;
  void beginAttempt(bool server, const QString &address, int port, bool tls, bool peerChecks);
  void processStateChanged(deskflow::core::ProcessState state);
  void connectionStateChanged(deskflow::core::ConnectionState state);
  void connectionIntentChanged(bool wanted);
  void secureSocket(bool enabled);
  void peerFingerprintObserved(); // An observed certificate is not a trusted identity.
  void retryIn(int seconds);
  void connectionRefused(deskflow::core::ConnectionRefusal reason);
  void requireAttention(Attention reason);
  void connectedClientsChanged(const QStringList &clients);
  // Discovery is an address search, distinct from transport/TLS connection state.
  void setDiscoveryActive(bool active);
  void setReconnectAllowed(bool allowed);
  void setIdleServerMode(bool server);
Q_SIGNALS:
  void reconnectRequested();
  void manualConnectionRequested();
  void settingsRequested();
  void recoverySettingsRequested();
  void summaryChanged(const QString &summary);

protected:
  void resizeEvent(QResizeEvent *event) override;
  void changeEvent(QEvent *event) override;

private:
  void render();
  void record(const QString &event);
  void refreshPeer();
  QString detail() const;
  QString phaseCode() const;
  bool m_server = false, m_tls = true, m_peerChecks = true, m_secure = false, m_fingerprint = false;
  bool m_active = false, m_discovering = false, m_retryPending = false, m_retryCycle = false, m_actionPending = false;
  bool m_reconnectAllowed = true, m_userPaused = false;
  QString m_address;
  int m_port = 0, m_clientCount = 0;
  Attention m_attention = Attention::None;
  deskflow::core::ProcessState m_process = deskflow::core::ProcessState::Stopped;
  deskflow::core::ConnectionState m_connection = deskflow::core::ConnectionState::Disconnected;
  QDeadlineTimer m_deadline;
  QJsonArray m_events;
  QTimer *m_timer;
  QLabel *m_state, *m_peer, *m_detail, *m_role, *m_metrics, *m_details;
  QPushButton *m_action, *m_showDetails, *m_export;
};
