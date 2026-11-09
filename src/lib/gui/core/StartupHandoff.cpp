// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "StartupHandoff.h"
#include "MacStartupExecutor.h"
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonDocument>
#include <QProcess>
#include <QRandomGenerator>
#include <QSharedMemory>
#include <QUuid>
#include <cstring>

namespace deskflow::gui::startup {
namespace {
const char workerOption[] = "--candidate-startup-transaction";
QByteArray line(QLocalSocket &s, int timeout)
{
  QElapsedTimer timer;
  timer.start();
  while (!s.canReadLine() && s.bytesAvailable() <= 2048 && timer.elapsed() < timeout &&
         s.state() == QLocalSocket::ConnectedState)
    s.waitForReadyRead(qMax(1, timeout - int(timer.elapsed())));
  if (s.bytesAvailable() > 2048 || !s.canReadLine())
    return {};
  return s.readLine(2049).trimmed();
}
bool noGuiInstance(const QString &key)
{
  if (key.isEmpty())
    return false;
  QSharedMemory memory(key);
  if (memory.attach(QSharedMemory::ReadOnly)) {
    memory.detach();
    return false;
  }
  // Permission/unknown errors are not evidence that the GUI has exited.
  return memory.error() == QSharedMemory::NotFound;
}
} // namespace
QJsonObject handoffRequest(const Plan &p, const QByteArray &confirmed)
{
  if (!p.valid() || confirmed != p.approvalDigest)
    return {};
  const auto canonical = prepare(p.before, p.options);
  if (canonical.approvalDigest != confirmed || canonical.operations != p.operations ||
      canonical.after.revision() != p.after.revision())
    return {};
  return {
      {"schema", 1},
      {"action", "apply"},
      {"digest", QString::fromLatin1(confirmed)},
      {"login", p.options.login},
      {"recovery", p.options.crashRecovery}
  };
}
bool checkHandoffRequest(const QJsonObject &request, const Snapshot &current, Plan &p)
{
  if (request.size() != 5 || request["schema"] != 1 || request["action"] != "apply" || !request["login"].isBool() ||
      !request["recovery"].isBool() || !request["digest"].isString())
    return false;
  p = prepare(current, {request["login"].toBool(), request["recovery"].toBool()});
  return p.valid() && p.approvalDigest == request["digest"].toString().toLatin1();
}
QJsonObject restoreHandoffRequest(const RecoveryRecord &r, const QByteArray &confirmed)
{
  if (!r.history.restorable || confirmed != r.history.restoreDigest)
    return {};
  return {
      {"schema", 1},
      {"action", "restore"},
      {"directory", r.history.directory},
      {"digest", QString::fromLatin1(confirmed)}
  };
}
bool startupWorkerRequested(int argc, char **argv)
{
  return argc > 1 && std::strcmp(argv[1], workerOption) == 0;
}
int runStartupWorker(const QStringList &args, const QString &instanceKey)
{
  if (!nativeStartupChangesAvailable() || args.size() != 4 || args[1] != workerOption || args[3].size() != 64 ||
      instanceKey.isEmpty())
    return 3;
  return runStartupWorkerSession(args, makeReadOnlyReader(), makeMacCommandRunner(), [instanceKey] {
    return noGuiInstance(instanceKey);
  });
}
int runStartupWorkerSession(
    const QStringList &args, std::shared_ptr<Reader> reader, std::shared_ptr<CommandRunner> commands,
    std::function<bool()> guiExited, int timeoutMs
)
{
  if (args.size() != 4 || args[1] != workerOption || args[3].size() != 64 || !reader || !commands || !guiExited ||
      timeoutMs < 1 || timeoutMs > 8000)
    return 3;
  // A transient copy of the same app; no QApplication, input core, permission
  // prompt, settings profile, daemon or independent network listener is opened.
  auto fail = [&](const QString &reason) {
    recordStartupHandoffFailure(reader->read(), reason);
    return 4;
  };
  QLocalSocket socket;
  socket.connectToServer(args[2]);
  if (!socket.waitForConnected(qMin(timeoutMs, 3000)))
    return fail("connect-failed");
  socket.write(args[3].toLatin1() + '\n');
  socket.flush();
  const auto request = QJsonDocument::fromJson(line(socket, timeoutMs)).object();
  Plan plan;
  RecoveryRecord recovery;
  auto validate = [&] {
    const auto current = reader->read();
    if (request["action"] == "apply")
      return checkHandoffRequest(request, current, plan);
    if (request.size() != 4 || request["schema"] != 1 || request["action"] != "restore" ||
        !request["directory"].isString() || !request["digest"].isString())
      return false;
    recovery = inspectRecoveryBackup(current, request["directory"].toString());
    return recovery.history.restorable && recovery.history.restoreDigest == request["digest"].toString().toLatin1();
  };
  if (!validate())
    return fail("request-or-state-mismatch");
  socket.write("ready\n");
  socket.flush();
  if (line(socket, timeoutMs) != "commit")
    return fail("commit-missing");
  if (socket.state() != QLocalSocket::UnconnectedState && !socket.waitForDisconnected(timeoutMs))
    return fail("gui-exit-timeout");
  if (!guiExited())
    return fail("gui-still-present");
  // Never interpret the IPC disconnect alone as consent; a matching request,
  // explicit commit, completed GUI exit and fresh plan are all required.
  if (!validate())
    return fail("state-changed-after-handoff");
  bool handoffComplete = true;
  MacStartupExecutor executor(reader, std::move(commands), [&] { return handoffComplete; });
  const auto result = request["action"] == "restore"
                          ? executor.restorePersisted(recovery.history.directory, recovery.history.restoreDigest, true)
                          : apply(plan, plan.approvalDigest, true, executor);
  handoffComplete = false;
  // Persistent journal is the source of truth on the next normal UI opening.
  return result.outcome == Outcome::Applied || result.outcome == Outcome::RolledBack ? 0
         : result.outcome == Outcome::FailedRolledBack                               ? 5
                                                                                     : 6;
}
StartupHandoff::StartupHandoff(QString key, QObject *parent) : QObject(parent), m_instanceKey(std::move(key))
{
  m_timeout.setSingleShot(true);
  connect(&m_timeout, &QTimer::timeout, this, [this] { fail("handoff-timeout"); });
  connect(&m_server, &QLocalServer::newConnection, this, [this] {
    while (m_server.hasPendingConnections()) {
      auto *peer = m_server.nextPendingConnection();
      if (m_peer) {
        peer->disconnectFromServer();
        peer->deleteLater();
        continue;
      }
      m_peer = peer;
      connect(peer, &QLocalSocket::readyRead, this, &StartupHandoff::receive);
      connect(peer, &QLocalSocket::disconnected, this, [this] {
        if (!m_ready)
          fail("handoff-disconnected");
      });
    }
  });
  connect(qApp, &QCoreApplication::aboutToQuit, this, [this] {
    if (m_ready && m_peer) {
      m_peer->write("commit\n");
      m_peer->flush();
      m_peer->waitForBytesWritten(200);
    }
  });
}
bool StartupHandoff::begin(const Plan &plan, const QByteArray &confirmed)
{
  return launch(handoffRequest(plan, confirmed), plan.before.executable);
}
bool StartupHandoff::beginRestore(const RecoveryRecord &record, const QByteArray &confirmed)
{
  return launch(restoreHandoffRequest(record, confirmed), record.plan.before.executable);
}
bool StartupHandoff::launch(const QJsonObject &request, const QString &reviewedExecutable)
{
  if (!nativeStartupChangesAvailable() || m_server.isListening() || m_instanceKey.isEmpty())
    return false;
  m_request = request;
  if (m_request.isEmpty())
    return false;
  QByteArray random(32, 0);
  for (auto &b : random)
    b = char(QRandomGenerator::system()->generate() & 255);
  m_nonce = QString::fromLatin1(random.toHex());
  m_server.setSocketOptions(QLocalServer::UserAccessOption);
  // Keep the Unix socket pathname within macOS limits even with its long
  // per-user temporary directory. This does not relax socket permissions.
  const auto name = "df-s-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
  if (!m_server.listen(name))
    return false;
  const auto executable = QCoreApplication::applicationFilePath();
  if (executable != reviewedExecutable ||
      !executable.endsWith("/DeskflowCandidate.app/Contents/MacOS/DeskflowCandidate")) {
    m_server.close();
    return false;
  }
  const auto bundle = executable.left(executable.size() - QString("/Contents/MacOS/DeskflowCandidate").size());
  // LaunchServices gives the transaction its own application launch. Do not
  // execute bootout from the GUI job that is about to be retired.
  if (!QProcess::startDetached("/usr/bin/open", {"-n", "-a", bundle, "--args", workerOption, name, m_nonce})) {
    m_server.close();
    return false;
  }
  m_timeout.start(12000);
  return true;
}
void StartupHandoff::receive()
{
  if (!m_peer || m_peer->bytesAvailable() > 2048) {
    fail("invalid-worker-response");
    return;
  }
  if (!m_peer->canReadLine())
    return;
  const auto reply = m_peer->readLine(2049).trimmed();
  if (!m_peer->property("authenticated").toBool()) {
    if (reply != m_nonce.toLatin1()) {
      fail("wrong-worker");
      return;
    }
    m_peer->setProperty("authenticated", true);
    m_peer->write(QJsonDocument(m_request).toJson(QJsonDocument::Compact) + '\n');
    m_peer->flush();
    return;
  }
  if (reply != "ready" || m_ready) {
    fail("unexpected-worker-response");
    return;
  }
  m_ready = true;
  m_timeout.stop();
  m_server.close();
  Q_EMIT readyToQuit();
}
void StartupHandoff::fail(const QString &reason)
{
  m_timeout.stop();
  m_ready = false;
  m_server.close();
  if (m_peer) {
    auto *peer = m_peer.data();
    m_peer.clear();
    peer->disconnect(this);
    peer->abort();
    peer->deleteLater();
  }
  Q_EMIT failed(reason);
}
} // namespace deskflow::gui::startup
