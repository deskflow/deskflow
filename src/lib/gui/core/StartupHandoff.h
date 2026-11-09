// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "MacStartupExecutor.h"
#include "StartupRecovery.h"
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QObject>
#include <QPointer>
#include <QTimer>

namespace deskflow::gui::startup {
// Wire data contains no configuration, certificate or private key. The worker
// independently reconstructs the canonical plan from its fresh observation.
QJsonObject handoffRequest(const Plan &, const QByteArray &confirmed);
QJsonObject restoreHandoffRequest(const RecoveryRecord &, const QByteArray &confirmed);
bool checkHandoffRequest(const QJsonObject &, const Snapshot &, Plan &);
bool startupWorkerRequested(int argc, char **argv);
int runStartupWorker(const QStringList &arguments, const QString &instanceKey);
// Same IPC/transaction implementation, with fake services in process tests.
int runStartupWorkerSession(
    const QStringList &, std::shared_ptr<Reader>, std::shared_ptr<CommandRunner>, std::function<bool()> guiExited,
    int timeoutMs = 8000
);

class StartupHandoff final : public QObject
{
  Q_OBJECT
public:
  explicit StartupHandoff(QString instanceKey, QObject *parent = nullptr);
  bool begin(const Plan &, const QByteArray &confirmed);
  bool beginRestore(const RecoveryRecord &, const QByteArray &confirmed);
  bool ready() const
  {
    return m_ready;
  }
Q_SIGNALS:
  void readyToQuit();
  void failed(const QString &reason);

private:
  void receive();
  void fail(const QString &reason);
  bool launch(const QJsonObject &, const QString &executable);
  QString m_instanceKey, m_nonce;
  QJsonObject m_request;
  QLocalServer m_server;
  QPointer<QLocalSocket> m_peer;
  QTimer m_timeout;
  bool m_ready = false;
};
} // namespace deskflow::gui::startup
