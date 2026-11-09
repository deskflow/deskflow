// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
// Deliberately does not link App, ClientApp, Computer, platform or Qt Widgets.
#include "arch/Arch.h"
#include "base/EventQueue.h"
#include "base/Log.h"
#include "common/CredentialStore.h"
#include "common/Enums.h"
#include "common/IsolatedSession.h"
#include "common/LabExecutionPolicy.h"
#include "common/PeerAuthentication.h"
#include "common/Settings.h"
#include "common/VersionInfo.h"
#include "gui/core/DiscoveryScope.h"
#include "net/NetworkAddress.h"
#include "net/ProbeInterfaceBinding.h"
#include "net/SecureListenSocket.h"
#include "net/SecureSocket.h"
#include "net/SocketMultiplexer.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QLockFile>
#include <QTextStream>
#include <mutex>

namespace {
// No argument, environment variable, Settings flag or IPC command can enable
// transport in this reviewed artifact. A separately reviewed build is needed.
constexpr bool kNetworkExecutionEnabled = deskflow::lab::kProbeNetworkExecutionEnabled;
std::mutex outputMutex;
void output(const QJsonObject &o)
{
  std::lock_guard lock(outputMutex);
  QTextStream(stdout) << QJsonDocument(o).toJson(QJsonDocument::Compact) << "\n";
}

// Compiled transport path reuses Deskflow TLS and trust checks. It exchanges no
// Deskflow input protocol messages, clipboard, key events or screen geometry.
class ScopedPeer final : public SecureSocket
{
public:
  using SecureSocket::SecureSocket;
  bool scope(quint32 index)
  {
    return deskflow::net::bindProbeInterface(getSocket(), index);
  }
};
class ScopedListener final : public SecureListenSocket
{
public:
  using SecureListenSocket::SecureListenSocket;
  bool scope(quint32 index)
  {
    return deskflow::net::bindProbeInterface(socket(), index);
  }
};
int transport(
    const deskflow::lab::IsolatedSession &session, const deskflow::auth::Context &context, quint32 interfaceIndex
)
{
  if (!deskflow::auth::Runtime::activate(context))
    return 3;
  Arch arch;
  arch.init();
  Log log;
  EventQueue events;
  SocketMultiplexer multiplexer;
  QLockFile lock(session.lockFile);
  lock.setStaleLockTime(0);
  if (!lock.tryLock(0))
    return 3;
  std::unique_ptr<IDataSocket> peer;
  std::unique_ptr<ScopedListener> listener;
  auto finish = [&](bool ok) { events.addEvent(Event(EventTypes::Quit, nullptr, new ExitEventData(ok ? 0 : 4))); };
  auto watch = [&] {
    auto *target = peer->getEventTarget();
    events.addHandler(EventTypes::DataSocketSecureConnected, target, [&](const auto &) { finish(true); });
    events.addHandler(EventTypes::ClientListenerAccepted, target, [&](const auto &) { finish(true); });
    for (auto type :
         {EventTypes::SocketDisconnected, EventTypes::DataSocketConnectionFailed,
          EventTypes::ClientListenerDisconnectedOnAccept})
      events.addHandler(type, target, [&](const auto &) { finish(false); });
  };
  NetworkAddress local(session.bindAddress.toStdString(), session.port);
  local.resolve();
  if (session.mode == "server") {
    listener = std::make_unique<ScopedListener>(
        &events, &multiplexer, IArchNetwork::AddressFamily::INet, SecurityLevel::PeerAuth
    );
    if (!listener->scope(interfaceIndex))
      return 3;
    events.addHandler(EventTypes::ListenSocketConnecting, listener->getEventTarget(), [&](const auto &) {
      if (peer)
        return;
      peer = listener->accept();
      if (!peer) {
        finish(false);
        return;
      }
      watch();
      listener->close();
    });
    listener->bind(local);
  } else {
    // Bind the explicit local address, with an OS-assigned source port. No
    // hostname discovery, wildcard bind or automatic interface fallback.
    ARCH->setAddrPort(local.getAddress(), 0);
    auto secure =
        std::make_unique<ScopedPeer>(&events, &multiplexer, IArchNetwork::AddressFamily::INet, SecurityLevel::PeerAuth);
    if (!secure->scope(interfaceIndex))
      return 3;
    secure->initSsl(false);
    secure->bind(local);
    peer = std::move(secure);
    watch();
    NetworkAddress remote(session.peerAddress.toStdString(), session.port);
    remote.resolve();
    peer->connect(remote);
  }
  auto *timer = events.newOneShotTimer(60, nullptr);
  events.addHandler(EventTypes::Timer, timer, [&](const auto &) { finish(false); });
  const auto result = events.loop();
  events.removeHandler(EventTypes::Timer, timer);
  events.deleteTimer(timer);
  if (peer) {
    events.removeHandlers(peer->getEventTarget());
    peer->close();
  }
  if (listener) {
    events.removeHandlers(listener->getEventTarget());
    listener->close();
  }
  return result;
}
} // namespace

// The normal core uses CoreIpcServer. This isolated executable deliberately
// sends only bounded authentication records to its parent through stdout.
void ipcSendToClient(const QString &command, const QString &args)
{
  if (command != "peerAuthentication")
    return;
  auto report = deskflow::auth::Report::decode(args);
  if (!report)
    return;
  output({{"event", "peerAuthentication"}, {"payload", args}});
}
void ipcSendConnectionState(deskflow::core::ConnectionState)
{
}

int main(int argc, char **argv)
{
  QCoreApplication app(argc, argv);
  QCoreApplication::setApplicationVersion(kVersion);
  auto args = app.arguments();
#ifdef DESKFLOW_CANDIDATE_BUILD
  auto parent = QDir::cleanPath(QStringLiteral(DESKFLOW_CANDIDATE_SESSIONS_ROOT));
  if (args.size() == 9 && args[7] == "--candidate-profile") {
    parent = deskflow::lab::CredentialStore::candidateSessionsDirectory(args[8]);
    if (parent.isEmpty())
      return 3;
    args = args.mid(0, 7);
  }
#else
  const auto parent = QDir::cleanPath(QStringLiteral(DESKFLOW_LAB_RUNS_ROOT) + "/../../dual-end-test");
#endif
  if (args.size() >= 3 && (args[1] == "--runtime-preflight" || args[1] == "--migration-preflight")) {
    bool ok = false;
    const auto profile = deskflow::lab::RuntimeProfile::decode(args[2].toUtf8(), &ok);
    if (args[1] == "--runtime-preflight" && args.size() == 3) {
      output(profile.preflight());
      return ok ? 0 : 3;
    }
    if (args[1] == "--migration-preflight" && args.size() == 4) {
      const auto migration = deskflow::lab::ProfileMigration::prepare(args[3].toUtf8(), profile);
      auto plan = migration.plan();
      plan["runtime"] = profile.preflight();
      plan["rollbackVerified"] = migration.valid() && migration.rollback(migration.after) == migration.before;
      output(plan);
      return ok && migration.valid() ? 0 : 3;
    }
    return 3;
  }
  if (args.size() == 3 && (args[1] == "--credentials-check" || args[1] == "--credentials-cleanup")) {
    QString error;
    if (args[1] == "--credentials-check") {
      auto lease = deskflow::lab::CredentialStore::acquire(args[2], parent, &error);
      output({{"valid", bool(lease)}, {"error", error}, {"networkExecuted", false}, {"credentialsCreated", false}});
      return lease ? 0 : 3;
    }
    error = deskflow::lab::CredentialStore::recover(args[2], parent);
    output({{"cleaned", error.isEmpty()}, {"error", error}, {"networkExecuted", false}});
    return error.isEmpty() ? 0 : 3;
  }
  if (args.size() == 2 && args[1] == "--describe") {
    output(
        {{"networkExecutionEnabled", kNetworkExecutionEnabled},
         {"inputEnabled", false},
         {"ipcListener", false},
         {"credentialsCreated", false},
         {"mode", "tls-only"}}
    );
    return 0;
  }
  if ((args.size() != 5 && args.size() != 7) || (args[1] != "--plan" && args[1] != "--run") ||
      args[3] != "--peer-auth-context")
    return 3;
  const auto session = deskflow::lab::IsolatedSession::inspect(args[2], parent);
  const auto context = deskflow::auth::Context::decode(args[4]);
  if (!session.valid() || !context.valid() || context.expected.isEmpty() ||
      context.session != QFileInfo(session.root).fileName()) {
    output({{"allowed", false}, {"reason", "invalid-isolation-or-identity"}, {"networkExecuted", false}});
    return 3;
  }
  auto plan = session.plan();
  plan["mode"] = "tls-only";
  plan["expectedIdentityRequired"] = true;
  plan["inputEnabled"] = false;
  plan["networkExecutionEnabled"] = kNetworkExecutionEnabled;
  if (args[1] == "--plan") {
    output(plan);
    return 0;
  }
  if (!kNetworkExecutionEnabled || !session.runtime.executable()) {
    output(
        {{"allowed", false},
         {"reason", "network-execution-not-authorized-in-this-build"},
         {"networkExecuted", false},
         {"credentialsCreated", false}}
    );
    return 3;
  }
  // Recheck the actual physical interface inside the child immediately before
  // credentials or sockets. A local address alone does not constrain routing.
  bool indexOk = false;
  const auto interfaceIndex = args.size() == 7 && args[5] == "--interface-index" ? args[6].toUInt(&indexOk) : 0;
  deskflow::gui::DiscoveryScope selected;
  if (indexOk && interfaceIndex && interfaceIndex <= 0xffffffu)
    for (const auto &scope : deskflow::gui::DiscoveryScope::current())
      if (scope.index == interfaceIndex && scope.address.toString() == session.bindAddress)
        selected = scope;
  const QHostAddress remote(session.peerAddress);
  const auto mask = selected.valid() ? 0xffffffffu >> selected.prefix : 0xffffffffu;
  const auto numeric = remote.toIPv4Address();
  if (!selected.valid() || !deskflow::gui::DiscoveryScope::privateAddress(remote) || remote == selected.address ||
      (numeric & ~mask) != (selected.address.toIPv4Address() & ~mask) || !(numeric & mask) || (numeric & mask) == mask)
    return 3;
  // The executable policy above remains closed. No automatic identity/trust
  // generation occurs here; credential provisioning requires a separate review.
  if (!deskflow::lab::IsolatedSession::activate(session.root, parent))
    return 3;
  QString error;
  auto lease = deskflow::lab::CredentialStore::acquire(session.root, parent, &error);
  if (!lease || !deskflow::lab::CredentialStore::activate(*lease))
    return 3;
  int result = 3;
  try {
    result = transport(session, context, interfaceIndex);
  } catch (...) {
    result = 3;
  } // Always release and clean this exact prepared session.

  lease.reset();
  const auto cleanup = deskflow::lab::CredentialStore::recover(session.root, parent);
  return cleanup.isEmpty() ? result : 3;
}
