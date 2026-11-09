// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "common/ExitCodes.h"
#include "common/ServiceSession.h"
#include "gui/core/CoreProcess.h"
#include "gui/core/NearbyDiscovery.h"
#include "gui/dialogs/NearbyDevicesDialog.h"
#include "gui/ipc/CoreIpcClient.h"
#include <QApplication>
#include <QDir>
#include <QListWidget>
#include <QPushButton>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

namespace deskflow::gui {
class NoSocketPreviousCore final : public ipc::CoreIpcClient
{
public:
  using ipc::CoreIpcClient::CoreIpcClient;
  int stops = 0;
  void sendStop() override
  {
    ++stops;
  }
};
class CoreProcessSessionTests : public QObject
{
  Q_OBJECT
  using State = deskflow::core::ProcessState;
  static ipc::CoreIpcClient *attach(CoreProcess &core, bool newGeneration = true)
  {
    if (newGeneration)
      core.invalidatePendingEvents();
    core.m_processState = State::Started;
    core.m_startRequested = true;
    auto *client = new ipc::CoreIpcClient(&core);
    core.m_coreIpcClient = client;
    core.bindCoreIpcClient(client);
    return client; // No socket connect; only actual queued signal delivery.
  }
  static void deliver()
  {
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
  }
private Q_SLOTS:
  void serverAddressObserverWorksWithoutDiscoveryAndNeverChoosesAnotherLink()
  {
    ServerConfig config;
    CoreProcess core(config);
    core.m_mode = Settings::CoreMode::Server;
    core.m_startRequested = true;
    core.m_processState = State::Started;
    Settings::setValue(Settings::Core::ProcessMode, Settings::ProcessMode::Desktop);
    DiscoveryScope a{7, "Fixture", QHostAddress("192.168.50.10"), 24}, b = a;
    b.address = QHostAddress("192.168.50.11");
    QVector<DiscoveryScope> scopes{a};
    auto source = [&] { return scopes; };
    Settings::setValue(Settings::Core::Interface, a.address.toString());
    core.observeServerInterfaces(source);
    QVERIFY(core.m_boundServerScope);
    auto other = b;
    other.index = 8;
    scopes = {other};
    core.observeServerInterfaces(source);
    QCOMPARE(Settings::value(Settings::Core::Interface).toString(), a.address.toString());
    auto second = b;
    second.address = QHostAddress("192.168.50.12");
    scopes = {b, second};
    core.observeServerInterfaces(source);
    QCOMPARE(Settings::value(Settings::Core::Interface).toString(), a.address.toString());
    scopes = {b};
    core.observeServerInterfaces(source);
    QCoreApplication::processEvents();
    QCOMPARE(Settings::value(Settings::Core::Interface).toString(), b.address.toString());
    core.stop(Settings::ProcessMode::Desktop);
    scopes = {a};
    core.observeServerInterfaces(source);
    QCOMPARE(Settings::value(Settings::Core::Interface).toString(), b.address.toString());
  }
  void serverRebindCommitsOnlyAfterOwnedCoreStops()
  {
    ServerConfig config;
    CoreProcess core(config);
    core.m_mode = Settings::CoreMode::Server;
    core.m_startRequested = true;
    core.m_processState = deskflow::core::ProcessState::Started;
    Settings::setValue(Settings::Core::ProcessMode, Settings::ProcessMode::Desktop);
    DiscoveryScope a{7, "Fixture", QHostAddress("192.168.50.10"), 24}, b = a;
    b.address = QHostAddress("192.168.50.11");
    QVector<DiscoveryScope> scopes{b};
    Settings::setValue(Settings::Core::Interface, a.address.toString());
    QVERIFY(core.rebindServer(a, b, [&] { return scopes; }));
    QCoreApplication::processEvents();
    QCOMPARE(Settings::value(Settings::Core::Interface).toString(), b.address.toString());
    QVERIFY(!core.m_serverRebind);
  }
  void serverRebindCancelledOrChangedInterfaceDoesNotCommit()
  {
    ServerConfig config;
    CoreProcess core(config);
    core.m_mode = Settings::CoreMode::Server;
    core.m_startRequested = true;
    core.m_processState = deskflow::core::ProcessState::Stopped;
    DiscoveryScope a{7, "Fixture", QHostAddress("192.168.50.10"), 24}, b = a;
    b.address = QHostAddress("192.168.50.11");
    Settings::setValue(Settings::Core::Interface, a.address.toString());
    auto wrong = b;
    wrong.index = 8;
    QVERIFY(!core.rebindServer(a, wrong, [&] { return QVector<DiscoveryScope>{wrong}; }));
    core.m_serverRebind = qMakePair(a, b);
    core.m_rebindScopes = [&] { return QVector<DiscoveryScope>{b}; };
    core.stop(Settings::ProcessMode::Desktop);
    core.finishServerRebind();
    QCOMPARE(Settings::value(Settings::Core::Interface).toString(), a.address.toString());
  }
  void serviceContextRoundTripStillRejectsWrongIdentityAndLateEvents()
  {
    ServerConfig config;
    CoreProcess core(config);
    const auto fp = QString(64, 'a');
    const auto context = core.labBeginAuthentication(fp);
    const deskflow::candidate::ServiceSession request{
        "/private/tmp/synthetic/DeskflowCandidate/DeskflowCandidate.conf", context
    };
    const auto daemon = deskflow::candidate::ServiceSession::decode(request.encode());
    QVERIFY(daemon.valid());
    QCOMPARE(daemon.context, context);
    QSignalSpy result(&core, &CoreProcess::authenticationResult);
    core.labDeliverAuthentication(
        deskflow::auth::Report{daemon.context, 1, deskflow::auth::Outcome::Started, {}}.encode()
    );
    core.labDeliverAuthentication(
        deskflow::auth::Report{daemon.context, 1, deskflow::auth::Outcome::WrongIdentity, QString(64, 'b')}.encode()
    );
    QCOMPARE(result.count(), 2);
    QVERIFY(core.m_recoveryBlocked);
    core.labDeliverAuthentication(
        deskflow::auth::Report{daemon.context, 2, deskflow::auth::Outcome::Verified, fp}.encode()
    );
    QCOMPARE(result.count(), 2);
    core.stop(Settings::ProcessMode::Desktop);
    core.labDeliverAuthentication(
        deskflow::auth::Report{daemon.context, 3, deskflow::auth::Outcome::Verified, fp}.encode()
    );
    QCOMPARE(result.count(), 2);
  }
  void serviceStopRequiresMatchingAcknowledgement()
  {
    ServerConfig config;
    CoreProcess core(config);
    core.m_processState = deskflow::core::ProcessState::Stopping;
    core.m_serviceStopToken = QString(32, 'a');
    core.serviceStopped(QString(32, 'b'));
    QCOMPARE(core.processState(), deskflow::core::ProcessState::Stopping);
    core.serviceStopped(QString(32, 'a'));
    QCOMPARE(core.processState(), deskflow::core::ProcessState::Stopped);
    QVERIFY(core.m_serviceStopToken.isEmpty());
    core.m_processState = deskflow::core::ProcessState::Started;
    core.serviceStopped(QString(32, 'a'));
    QCOMPARE(core.processState(), deskflow::core::ProcessState::Started);
  }

  void previousGuiCoreIsRetiredBeforeFreshSession()
  {
    ServerConfig config;
    CoreProcess core(config);
    core.m_startRequested = true;
    auto *previous = new NoSocketPreviousCore(&core);
    core.bindExistingProcess(previous);
    Q_EMIT previous->connected();
    QCOMPARE(previous->stops, 1);
    QVERIFY(!core.m_retryTimer.isActive());
    Q_EMIT previous->serverShutdown();
    QCOMPARE(core.processState(), State::RetryPending);
    QVERIFY(core.m_retryTimer.isActive());
    core.stop(Settings::ProcessMode::Desktop);
    QVERIFY(!core.m_retryTimer.isActive());
    QVERIFY(!core.connectionRequested());
  }
  void previousCoreSignalsCannotUndoPauseOrNewGeneration()
  {
    ServerConfig config;
    CoreProcess core(config);
    core.m_startRequested = true;
    auto *previous = new NoSocketPreviousCore(&core);
    core.bindExistingProcess(previous);
    core.invalidatePendingEvents();
    Q_EMIT previous->connected();
    Q_EMIT previous->serverShutdown();
    QCOMPARE(previous->stops, 0);
    QVERIFY(!core.m_retryTimer.isActive());
    auto *paused = new NoSocketPreviousCore(&core);
    core.bindExistingProcess(paused);
    core.m_startRequested = false;
    Q_EMIT paused->versionMismatch();
    Q_EMIT paused->serverShutdown();
    QCOMPARE(paused->stops, 0);
    QVERIFY(!core.m_retryTimer.isActive());
  }
  void unresponsivePreviousCoreFailsClosed()
  {
    ServerConfig config;
    CoreProcess core(config);
    core.m_startRequested = true;
    auto *previous = new NoSocketPreviousCore(&core);
    core.bindExistingProcess(previous);
    QSignalSpy error(&core, &CoreProcess::error);
    auto *deadline = previous->findChild<QTimer *>("existingCoreStopDeadline");
    QVERIFY(deadline);
    QVERIFY(QMetaObject::invokeMethod(deadline, "timeout", Qt::DirectConnection));
    QCOMPARE(error.count(), 1);
    QVERIFY(core.m_recoveryBlocked);
    QVERIFY(!core.m_retryTimer.isActive());
    Q_EMIT previous->serverShutdown();
    Q_EMIT previous->connected();
    QCOMPARE(previous->stops, 0);
    QVERIFY(!core.m_retryTimer.isActive());
  }
  void discoveryRejectsInvalidOrNonlocalEndpoints_data()
  {
    QTest::addColumn<QString>("host");
    QTest::addColumn<int>("port");
    QTest::addColumn<bool>("accepted");
    QTest::newRow("local") << "work-pc.local." << 24800 << true;
    QTest::newRow("uppercase") << "WORK-PC.LOCAL" << 34800 << true;
    QTest::newRow("external") << "example.com" << 24800 << false;
    QTest::newRow("zero-port") << "pc.local" << 0 << false;
    QTest::newRow("overflow") << "pc.local" << 65536 << false;
    QTest::newRow("url") << "https://pc.local" << 24800 << false;
    QTest::newRow("control") << "pc.local\n" << 24800 << false;
    QTest::newRow("long-label") << (QString(64, 'x') + ".local") << 24800 << false;
  }
  void discoveryRejectsInvalidOrNonlocalEndpoints()
  {
    QFETCH(QString, host);
    QFETCH(int, port);
    QFETCH(bool, accepted);
    QCOMPARE(NearbyDiscovery::validEndpoint(host, port), accepted);
  }
  void discoverySelectionDoesNotConnectOrTrust()
  {
    NearbyDevicesDialog dialog;
    auto *provider = dialog.findChild<NearbyDiscovery *>();
    QVERIFY(provider);
    auto *devices = dialog.findChild<QListWidget *>("nearbyDevices");
    QVERIFY(devices);
    auto *button = dialog.findChild<QPushButton *>("selectNearbyAddress");
    QVERIFY(button);
    QVERIFY(!button->isEnabled());
    Q_EMIT provider->found("one", "Untrusted device", "pc.local.", 24800);
    QCOMPARE(devices->count(), 1);
    devices->setCurrentRow(0);
    QVERIFY(button->isEnabled());
    QCOMPARE(dialog.selectedHost(), QString("pc.local."));
    Q_EMIT provider->found("bad", "Spoof", "https://remote.invalid", 24800);
    QCOMPARE(devices->count(), 1);
    Q_EMIT provider->removed("one");
    QCOMPARE(devices->count(), 0);
    QVERIFY(!button->isEnabled());
    QCOMPARE(dialog.result(), int(QDialog::Rejected));
  }
  void processExitRecovery_data()
  {
    QTest::addColumn<int>("code");
    QTest::addColumn<bool>("crash");
    QTest::addColumn<bool>("wanted");
    QTest::addColumn<bool>("retry");
    QTest::newRow("network-core-ended") << s_exitSuccess << false << true << true;
    QTest::newRow("crash") << 9 << true << true << true;
    QTest::newRow("bad-config") << s_exitConfig << false << true << false;
    QTest::newRow("bad-args") << s_exitArgs << false << true << false;
    QTest::newRow("generic-fatal") << s_exitFailed << false << true << false;
    QTest::newRow("stopped-normal") << s_exitSuccess << false << false << false;
    QTest::newRow("stopped-crash") << 9 << true << false << false;
  }
  void processExitRecovery()
  {
    QFETCH(int, code);
    QFETCH(bool, crash);
    QFETCH(bool, wanted);
    QFETCH(bool, retry);
    ServerConfig config(5, 3);
    CoreProcess core(config);
    attach(core);
    core.m_startRequested = wanted;
    core.onProcessFinished(code, crash ? QProcess::CrashExit : QProcess::NormalExit);
    QCOMPARE(core.processState(), retry ? State::RetryPending : State::Stopped);
    QCOMPARE(core.m_retryTimer.isActive(), retry);
    core.stop(Settings::ProcessMode::Desktop);
    QVERIFY(!core.m_retryTimer.isActive());
  }
  void crashLoopHasBoundedAutomaticRecovery()
  {
    ServerConfig config(5, 3);
    CoreProcess core(config);
    for (int i = 0; i < 3; ++i) {
      attach(core);
      core.onProcessFinished(9, QProcess::CrashExit);
      QCOMPARE(core.processState(), State::RetryPending);
    }
    attach(core);
    core.onProcessFinished(9, QProcess::CrashExit);
    QCOMPARE(core.processState(), State::Stopped);
    QVERIFY(core.m_recoveryBlocked);
  }
  void terminalRefusalNeverSchedulesRecovery()
  {
    ServerConfig config(5, 3);
    CoreProcess core(config);
    auto *client = attach(core);
    core.setMode(Settings::CoreMode::Client);
    QSignalSpy retry(&core, &CoreProcess::retryIn);
    Q_EMIT client->commandReceived("connectionRefused", "IncompatibleVersion");
    deliver();
    Q_EMIT client->commandReceived("retryIn", "10");
    deliver();
    QCOMPARE(retry.count(), 0);
    core.onProcessFinished(s_exitSuccess, QProcess::NormalExit);
    QCOMPARE(core.processState(), State::Stopped);
    QVERIFY(!core.m_retryTimer.isActive());
  }
  void malformedRetryTimesAreIgnored()
  {
    ServerConfig config(5, 3);
    CoreProcess core(config);
    auto *client = attach(core);
    QSignalSpy retry(&core, &CoreProcess::retryIn);
    for (const auto &value : QStringList{"-1", "999999999999", "5x", "3601"})
      Q_EMIT client->commandReceived("retryIn", value);
    deliver();
    QCOMPARE(retry.count(), 0);
    Q_EMIT client->commandReceived("retryIn", "5");
    deliver();
    QCOMPARE(retry.count(), 1);
  }
  void currentSessionDelivers()
  {
    ServerConfig config(5, 3);
    CoreProcess core(config);
    auto *client = attach(core);
    QSignalSpy security(&core, &CoreProcess::secureSocket);
    QSignalSpy state(&core, &CoreProcess::connectionStateObserved);
    Q_EMIT client->commandReceived("secureSocket", "TLSv1.3");
    Q_EMIT client->commandReceived("connectionState", "Connected");
    QCOMPARE(security.count(), 0);
    deliver();
    QCOMPARE(security.count(), 1);
    QCOMPARE(state.count(), 1);
    QCOMPARE(core.connectionState(), deskflow::core::ConnectionState::Connected);
  }
  void oldQueuedMessagesAfterRestart_data()
  {
    QTest::addColumn<QString>("command");
    QTest::addColumn<QString>("value");
    QTest::newRow("connection") << "connectionState" << "Connected";
    QTest::newRow("security") << "secureSocket" << "TLSv1.3";
    QTest::newRow("fingerprint") << "peerFingerprint" << "synthetic-not-a-real-key";
    QTest::newRow("retry") << "retryIn" << "30";
    QTest::newRow("clients") << "connectedClients" << "OldSyntheticClient";
    QTest::newRow("refusal") << "connectionRefused" << "DuplicateName";
  }
  void oldQueuedMessagesAfterRestart()
  {
    QFETCH(QString, command);
    QFETCH(QString, value);
    ServerConfig config(5, 3);
    CoreProcess core(config);
    auto *old = attach(core);
    QSignalSpy state(&core, &CoreProcess::connectionStateObserved);
    QSignalSpy tls(&core, &CoreProcess::secureSocket);
    QSignalSpy pin(&core, &CoreProcess::peerFingerprint);
    QSignalSpy retry(&core, &CoreProcess::retryIn);
    QSignalSpy clients(&core, &CoreProcess::connectedClientsChanged);
    QSignalSpy refusal(&core, &CoreProcess::connectionRefused);
    Q_EMIT old->commandReceived(command, value);
    auto *current = attach(core); // New process is also Started when old event arrives.
    deliver();
    QCOMPARE(state.count() + tls.count() + pin.count() + retry.count() + clients.count() + refusal.count(), 0);
    Q_EMIT current->commandReceived("retryIn", "5");
    deliver();
    QCOMPARE(retry.count(), 1);
  }
  void generationMattersEvenWithSamePointer()
  {
    ServerConfig config(5, 3);
    CoreProcess core(config);
    auto *client = attach(core);
    QSignalSpy spy(&core, &CoreProcess::secureSocket);
    Q_EMIT client->commandReceived("secureSocket", "TLSv1.3");
    core.invalidatePendingEvents();
    deliver();
    QCOMPARE(spy.count(), 0);
  }
  void replacedSourceWithSameGenerationIsIgnored()
  {
    ServerConfig config(5, 3);
    CoreProcess core(config);
    auto *old = attach(core);
    QSignalSpy spy(&core, &CoreProcess::secureSocket);
    Q_EMIT old->commandReceived("secureSocket", "TLSv1.3");
    attach(core, false);
    deliver();
    QCOMPARE(spy.count(), 0);
  }
  void deletedSourceIsIgnored()
  {
    ServerConfig config(5, 3);
    CoreProcess core(config);
    auto *client = attach(core);
    QSignalSpy spy(&core, &CoreProcess::secureSocket);
    Q_EMIT client->commandReceived("secureSocket", "TLSv1.3");
    delete client;
    core.m_coreIpcClient = nullptr;
    deliver();
    QCOMPARE(spy.count(), 0);
  }
  void stoppedSessionCannotPublish()
  {
    ServerConfig config(5, 3);
    CoreProcess core(config);
    auto *client = attach(core);
    QSignalSpy spy(&core, &CoreProcess::secureSocket);
    Q_EMIT client->commandReceived("secureSocket", "TLSv1.3");
    core.m_processState = State::Stopped;
    deliver();
    QCOMPARE(spy.count(), 0);
  }
  void oldShutdownDoesNotClearNewClient()
  {
    ServerConfig config(5, 3);
    CoreProcess core(config);
    auto *old = attach(core);
    auto *current = attach(core);
    Q_EMIT old->serverShutdown();
    QCOMPARE(core.m_coreIpcClient, current);
  }
  void stopInvalidatesQueuedCommandsAndRetry()
  {
    ServerConfig config(5, 3);
    CoreProcess core(config);
    auto *client = attach(core);
    QSignalSpy spy(&core, &CoreProcess::secureSocket);
    Q_EMIT client->commandReceived("secureSocket", "TLSv1.3");
    core.m_process = new QProcess(&core);
    core.m_retryTimer.start(10000);
    core.stop(Settings::ProcessMode::Desktop);
    QVERIFY(!core.m_retryTimer.isActive());
    attach(core);
    deliver();
    QCOMPARE(spy.count(), 0);
  }
};
} // namespace deskflow::gui

int main(int argc, char **argv)
{
  const auto root = qEnvironmentVariable("DESKFLOW_TEST_ROOT");
  if (root.isEmpty() || !QDir(root).isAbsolute() || !QDir(root).exists())
    return 2;
  QTemporaryDir fixture(root + "/core-session-XXXXXX");
  if (!fixture.isValid())
    return 2;
  qputenv("XDG_CONFIG_HOME", (fixture.path() + "/config").toUtf8());
  qputenv("XDG_STATE_HOME", (fixture.path() + "/state").toUtf8());
  QDir().mkpath(fixture.path() + "/config/DeskflowLab");
  QDir().mkpath(fixture.path() + "/state");
  QApplication app(argc, argv);
  Settings::setValue(Settings::Core::ComputerName, "SyntheticLab");
  Settings::setValue(Settings::Core::ProcessMode, Settings::ProcessMode::Desktop);
  deskflow::gui::CoreProcessSessionTests tests;
  return QTest::qExec(&tests, argc, argv);
}
#include "CoreProcessSessionTests.moc"
