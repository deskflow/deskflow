/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2024 - 2025 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once
#include "common/WorkspaceApply.h"

#include "DiscoveryScope.h"
#include "common/Enums.h"
#include "common/PeerAuthentication.h"
#include "common/Settings.h"
#include "gui/FileTail.h"
#include "gui/config/ServerConfig.h"

#include <QMutex>
#include <QObject>
#include <QProcess>
#include <QTimer>
class DesktopProductFlowTests;

namespace deskflow::gui {

namespace ipc {
class CoreIpcClient;
class DaemonIpcClient;
} // namespace ipc

class CoreProcess : public QObject
{
  using ConnectionState = deskflow::core::ConnectionState;
  using ProcessMode = Settings::ProcessMode;
  using ProcessState = deskflow::core::ProcessState;
  Q_OBJECT

public:
  QString applyWorkspace(const QByteArray &state);
  enum class Error
  {
    AddressMissing,
    StartFailed
  };

  explicit CoreProcess(const ServerConfig &serverConfig);

  bool rebindServer(
      const DiscoveryScope &before, const DiscoveryScope &after, DiscoveryScopeSource source = DiscoveryScope::current
  );
  bool connectionRequested() const
  {
    return m_startRequested;
  }
  bool recoveryPermitted() const;
  bool recoveryQuiescent() const;
  bool holdForAddressRecovery();
  bool resumeForAddressRecovery(const QString &fingerprint, const QString &address, int port);
  void start(std::optional<ProcessMode> processMode = std::nullopt);
  void stop(std::optional<ProcessMode> processMode = std::nullopt);
  void restart();
  void cleanup();
  void applyLogLevel();
  void clearSettings();
  void retryDaemon();
  void observeServerInterfaces(DiscoveryScopeSource source = DiscoveryScope::current);
  bool setExpectedPeerFingerprint(const QString &fingerprint);
  QString expectedPeerFingerprint() const
  {
    return m_expectedPeer;
  }
#ifdef DESKFLOW_LAB_BUILD
  void labObserve(ProcessState process, ConnectionState connection)
  {
    setProcessState(process);
    setConnectionState(connection);
  }
  deskflow::auth::Context labBeginAuthentication(const QString &expected);
  void labDeliverAuthentication(const QString &encoded)
  {
    onCoreIpcMessageReceived("peerAuthentication", encoded);
  }
#endif

  // getters
  Settings::CoreMode mode() const
  {
    return m_mode;
  }
  QString secureSocketVersion() const
  {
    return m_secureSocketVersion;
  }
  bool isStarted() const
  {
    return m_processState == ProcessState::Started;
  }
  ProcessState processState() const
  {
    return m_processState;
  }
  ConnectionState connectionState() const
  {
    return m_connectionState;
  }

  // setters
  void setAddress(const QString &address)
  {
    m_address = correctedAddress(address);
  }
  void setMode(Settings::CoreMode mode)
  {
    m_mode = mode;
  }

Q_SIGNALS:
  void connectionIntentChanged(bool wanted);
  void inputExecutionUnavailable();
  void recoveredConnectionRequested(const QString &fingerprint, const QString &address, int port);
  void powerStateChanged(bool sleeping);
  void error(deskflow::gui::CoreProcess::Error error);
  void logLine(const QString &line);
  void connectionStateChanged(deskflow::core::ConnectionState state);
  void connectionStateObserved(deskflow::core::ConnectionState state);
  void processStateChanged(deskflow::core::ProcessState state);
  void secureSocket(bool enabled);
  // Snapshot the desktop client's settings immediately before launch, not edited UI text.
  void clientConnectionStarting(const QString &address, int port, bool tlsAndPeerChecksEnabled);
  void connectionAttemptStarting(bool server, const QString &address, int port, bool tls, bool peerChecks);
  void daemonIpcClientConnectionFailed();
  void connectedClientsChanged(const QStringList &clients);
  void displayTopologyReceived(const QString &device, const QByteArray &topology);
  // Fingerprint identifies only the verified server that supplied the view;
  // other devices in it are display metadata, never newly trusted peers.
  void
  workspaceSnapshotReceived(const QString &serverDevice, const QByteArray &snapshot, const QString &serverFingerprint);
  void workspaceApplyFinished(const QString &request, const QString &outcome, const QString &reason);
  void verifiedPeerNameReceived(const QString &fingerprint, const QString &name);
  void monitorRoutingModeChanged(bool perMonitor);
  void securityLevelChanged(QString securityLevel);
  void unrecognisedClient(const QString &clientName);
  void connectionRefused(deskflow::core::ConnectionRefusal reason);
  void retryIn(int seconds);
  void peerFingerprint(const QString &fingerprint);
  void authenticationStarting(const deskflow::auth::Context &context, const QString &address, int port);
  void authenticationResult(const deskflow::auth::Report &report);
  void authenticationCancelled();
  void missingKeyboardLayouts(const QString &layouts);

private Q_SLOTS:
  void onProcessFinished(int exitCode, QProcess::ExitStatus);
  void onProcessReadyReadStandardOutput();
  void onProcessReadyReadStandardError();
  void onCoreIpcMessageReceived(const QString &command, const QString &args);
  void daemonIpcClientConnected();

private:
  friend class ::DesktopProductFlowTests;
  friend class CoreProcessSessionTests;
  friend class AutomaticRecoveryTests;
  friend class AuthenticationChainTests;
  deskflow::auth::Context prepareAuthentication();
  void bindCoreIpcClient(ipc::CoreIpcClient *client);
  void invalidatePendingEvents();
  void stopInternal(std::optional<ProcessMode>, bool preserveIntent);
  void startForegroundProcess(const QStringList &args);
  void startProcessFromDaemon();
  void stopForegroundProcess();
  void stopProcessFromDaemon();
  QPair<bool, QString> persistServerConfig() const;
  void setConnectionState(ConnectionState state);
  void setProcessState(ProcessState state);
  bool checkSecureSocket(const QString &line);
  void handleLogLines(const QString &text);
  QString correctedAddress(const QString &address) const;
  void setupDaemonLogTail(const QString &logPath);
  void checkExistingProcess();
  void bindExistingProcess(ipc::CoreIpcClient *client);
  int m_existingCoreAttempts = 0;
  std::optional<QPair<DiscoveryScope, DiscoveryScope>> m_serverRebind;
  std::optional<DiscoveryScope> m_boundServerScope;
  DiscoveryScopeSource m_rebindScopes;
  void finishServerRebind();
  QTimer m_rebindDeadline;
  static QString makeQuotedArgs(const QString &app, const QStringList &args);
  static QString processModeToString(const Settings::ProcessMode mode);
  static QString processStateToString(const CoreProcess::ProcessState state);
  static QString wrapIpv6(const QString &address);

  const ServerConfig &m_serverConfig;
  QString m_address;
  ProcessState m_processState = ProcessState::Stopped;
  ConnectionState m_connectionState = ConnectionState::Disconnected;
  Settings::CoreMode m_mode = Settings::CoreMode::None;
  QMutex m_processMutex;
  QString m_secureSocketVersion;
  std::optional<ProcessMode> m_lastProcessMode = std::nullopt;
  QTimer m_retryTimer;
  quint64 m_runGeneration = 0;
  std::optional<deskflow::display::WorkspaceSnapshot> m_workspaceLive;
  std::optional<deskflow::display::WorkspaceApplyRequest> m_workspacePending;
  deskflow::auth::Gate m_authGate;
  QString m_expectedPeer, m_serviceContext, m_serviceStopToken;
  QString m_authenticatedPeer;
  void serviceStopped(const QString &token);
  bool m_authRequired = false, m_peerVerified = false;
  quint64 m_restartRequest = 0;
  bool m_startRequested = false;
  bool m_recoveryBlocked = false;
  bool m_addressRecoveryHold = false;
  int m_crashRetries = 0;
  deskflow::gui::ipc::CoreIpcClient *m_coreIpcClient = nullptr;
  deskflow::gui::ipc::DaemonIpcClient *m_daemonIpcClient = nullptr;
  FileTail *m_daemonFileTail = nullptr;
  QProcess *m_process = nullptr;
  QString m_appPath;
};

} // namespace deskflow::gui
