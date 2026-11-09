// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "gui/dialogs/DiagnosticExportDialog.h"
#include "gui/widgets/ConnectionDiagnostics.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>
using deskflow::core::ConnectionRefusal;
using deskflow::core::ConnectionState;
using deskflow::core::ProcessState;
using Phase = ConnectionDiagnostics::Phase;
using Attention = ConnectionDiagnostics::Attention;
class ConnectionDiagnosticsTests : public QObject
{
  Q_OBJECT
  void start(ConnectionDiagnostics &d, bool tls = true, bool peers = true)
  {
    d.beginAttempt(false, "example-server.invalid", 24800, tls, peers);
    d.processStateChanged(ProcessState::Started);
    d.connectionStateChanged(ConnectionState::Connecting);
  }
private Q_SLOTS:
  void detailsRevealMetricsAndExportWithoutDuplicatingDiscovery()
  {
    ConnectionDiagnostics d;
    d.show();
    d.setDiscoveryActive(true);
    auto *metrics = d.findChild<QLabel *>("diagnosticMetrics");
    auto *exportButton = d.findChild<QPushButton *>("diagnosticExportButton");
    QVERIFY(metrics->isHidden());
    QVERIFY(exportButton->isHidden());
    start(d);
    auto *details = d.findChild<QPushButton *>("diagnosticDetailsButton");
    for (int i = 0; i < 3; ++i) {
      details->click();
      QVERIFY(metrics->isVisible());
      QVERIFY(exportButton->isVisible());
      QVERIFY(metrics->text().contains("not measured"));
      details->click();
      QVERIFY(metrics->isHidden());
      QVERIFY(exportButton->isHidden());
    }
  }
  void userPauseAndRecoveryActionRemainDistinctAndExportable()
  {
    ConnectionDiagnostics d;
    start(d);
    d.connectionIntentChanged(false);
    d.processStateChanged(ProcessState::Stopped);
    QCOMPARE(d.phase(), Phase::Paused);
    QVERIFY(!DiagnosticExportDialog::sanitized(d.diagnosticSnapshot()).isEmpty());
    start(d);
    d.requireAttention(Attention::Recovery);
    QSignalSpy recovery(&d, &ConnectionDiagnostics::recoverySettingsRequested),
        general(&d, &ConnectionDiagnostics::settingsRequested);
    d.findChild<QPushButton *>("diagnosticAction")->click();
    QCOMPARE(recovery.count(), 1);
    QCOMPARE(general.count(), 0);
    const auto data = DiagnosticExportDialog::sanitized(d.diagnosticSnapshot());
    QVERIFY(!data.isEmpty());
    QVERIFY(data.contains("attention-Recovery"));
  }
  void processAndPortAreNotConnectionSuccess()
  {
    ConnectionDiagnostics d;
    start(d);
    QCOMPARE(d.phase(), Phase::Connecting);
    d.secureSocket(true);
    QCOMPARE(d.phase(), Phase::Verifying);
    QVERIFY(!d.diagnosticSnapshot().contains("inputDeliveryVerified\":true"));
    d.connectionStateChanged(ConnectionState::Connected);
    QCOMPARE(d.phase(), Phase::Connected);
  }
  void reorderedTlsConfirmationDoesNotPrematurelyClaimConnected()
  {
    ConnectionDiagnostics d;
    start(d);
    d.connectionStateChanged(ConnectionState::Connected);
    QCOMPARE(d.phase(), Phase::Verifying);
    d.secureSocket(true);
    QCOMPARE(d.phase(), Phase::Connected);
  }
  void observedFingerprintDoesNotGrantTrust()
  {
    ConnectionDiagnostics d;
    start(d);
    d.peerFingerprintObserved();
    QCOMPARE(d.phase(), Phase::Verifying);
    d.requireAttention(Attention::Identity);
    d.retryIn(3);
    QCOMPARE(d.phase(), Phase::NeedsAction);
    d.processStateChanged(ProcessState::Stopped);
    QCOMPARE(d.phase(), Phase::NeedsAction);
    d.secureSocket(true);
    d.connectionStateChanged(ConnectionState::Connected);
    QCOMPARE(d.phase(), Phase::NeedsAction);
    start(d);
    QCOMPARE(d.phase(), Phase::Connecting);
  }
  void securityDowngradeCannotAppearSecure_data()
  {
    QTest::addColumn<bool>("tls");
    QTest::addColumn<bool>("peers");
    QTest::newRow("tls-off") << false << true;
    QTest::newRow("peers-off") << true << false;
    QTest::newRow("both-off") << false << false;
  }
  void securityDowngradeCannotAppearSecure()
  {
    QFETCH(bool, tls);
    QFETCH(bool, peers);
    ConnectionDiagnostics d;
    start(d, tls, peers);
    d.secureSocket(true);
    d.connectionStateChanged(ConnectionState::Connected);
    QCOMPARE(d.phase(), Phase::NeedsAction);
  }
  void lateEventsAfterStopCannotResurrectConnection()
  {
    ConnectionDiagnostics d;
    start(d);
    d.secureSocket(true);
    d.connectionStateChanged(ConnectionState::Connected);
    d.processStateChanged(ProcessState::Stopping);
    QCOMPARE(d.phase(), Phase::Stopping);
    d.connectionStateChanged(ConnectionState::Connected);
    d.retryIn(1);
    d.secureSocket(true);
    d.processStateChanged(ProcessState::Stopped);
    QCOMPARE(d.phase(), Phase::Idle);
  }
  void retryDeadlineDoesNotImplyNetworkSuccess()
  {
    ConnectionDiagnostics d;
    start(d);
    d.connectionStateChanged(ConnectionState::Disconnected);
    d.retryIn(1);
    QCOMPARE(d.phase(), Phase::Recovering);
    QVERIFY(d.findChild<QLabel *>("diagnosticReason")->text().contains("1 s"));
    QTRY_COMPARE_WITH_TIMEOUT(
        d.findChild<QLabel *>("diagnosticReason")->text(),
        QString("Retrying now. The previous attempt did not complete; identity is checked again before input starts."),
        3000
    );
    QCOMPARE(d.phase(), Phase::Recovering);
    d.connectionStateChanged(ConnectionState::Connecting);
    QCOMPARE(d.phase(), Phase::Recovering);
  }
  void freshRetryNeedsFreshTls()
  {
    ConnectionDiagnostics d;
    start(d);
    d.secureSocket(true);
    d.connectionStateChanged(ConnectionState::Connected);
    d.connectionStateChanged(ConnectionState::Disconnected);
    d.retryIn(0);
    d.connectionStateChanged(ConnectionState::Connecting);
    d.connectionStateChanged(ConnectionState::Connected);
    QCOMPARE(d.phase(), Phase::Recovering);
    d.secureSocket(true);
    QCOMPARE(d.phase(), Phase::Connected);
  }
  void repeatedConnectingObservationEndsRetryDisplay()
  {
    ConnectionDiagnostics d;
    start(d);
    d.retryIn(30);
    QCOMPARE(d.phase(), Phase::Recovering);
    d.connectionStateChanged(ConnectionState::Connecting);
    QCOMPARE(d.phase(), Phase::Recovering);
  }
  void retryCycleUsesOneStableRecoveringPhase()
  {
    ConnectionDiagnostics d;
    start(d);
    d.secureSocket(true);
    d.connectionStateChanged(ConnectionState::Disconnected);
    d.retryIn(12);
    QCOMPARE(d.phase(), Phase::Recovering);
    QCOMPARE(d.summary(), QString("Reconnecting and checking identity"));
    d.processStateChanged(ProcessState::RetryPending);
    QCOMPARE(d.phase(), Phase::Recovering);
    d.processStateChanged(ProcessState::Started);
    d.connectionStateChanged(ConnectionState::Connecting);
    QCOMPARE(d.phase(), Phase::Recovering);
    d.peerFingerprintObserved();
    d.secureSocket(true);
    QCOMPARE(d.phase(), Phase::Recovering);
    d.connectionStateChanged(ConnectionState::Connected);
    QCOMPARE(d.phase(), Phase::Connected);
  }
  void restartingCoreDuringRetryKeepsOneRecoveryPhase()
  {
    ConnectionDiagnostics d;
    start(d);
    d.connectionStateChanged(ConnectionState::Disconnected);
    d.processStateChanged(ProcessState::RetryPending);
    QCOMPARE(d.phase(), Phase::Recovering);
    d.processStateChanged(ProcessState::Stopped);
    QCOMPARE(d.phase(), Phase::Recovering);
    d.beginAttempt(false, "example-server.invalid", 24800, true, true);
    QCOMPARE(d.phase(), Phase::Recovering);
    d.processStateChanged(ProcessState::Started);
    d.connectionStateChanged(ConnectionState::Connecting);
    d.secureSocket(true);
    QCOMPARE(d.phase(), Phase::Recovering);
    d.connectionStateChanged(ConnectionState::Connected);
    QCOMPARE(d.phase(), Phase::Connected);
  }
  void acceptedServerIdentityResumesListeningWithoutRestart()
  {
    ConnectionDiagnostics d;
    d.beginAttempt(true, "", 24800, true, true);
    d.processStateChanged(ProcessState::Started);
    d.connectionStateChanged(ConnectionState::Listening);
    d.requireAttention(Attention::Identity);
    QCOMPARE(d.phase(), Phase::NeedsAction);
    d.requireAttention(Attention::None);
    QCOMPARE(d.phase(), Phase::Listening);
    d.connectedClientsChanged({"Client"});
    QCOMPARE(d.phase(), Phase::Connected);
  }
  void unavailableCoreActionCannotStartOrGetStuck()
  {
    ConnectionDiagnostics d;
    // Idle opens the real address editor; it must not start the core.
    QSignalSpy manual(&d, &ConnectionDiagnostics::manualConnectionRequested);
    QSignalSpy idleReconnect(&d, &ConnectionDiagnostics::reconnectRequested);
    d.findChild<QPushButton *>("diagnosticAction")->click();
    QCOMPARE(manual.count(), 1);
    QCOMPARE(idleReconnect.count(), 0);
    start(d);
    d.setReconnectAllowed(false);
    QSignalSpy reconnect(&d, &ConnectionDiagnostics::reconnectRequested);
    auto *button = d.findChild<QPushButton *>("diagnosticAction");
    button->click();
    QCOMPARE(reconnect.count(), 0);
    d.setReconnectAllowed(true);
    QVERIFY(button->isEnabled());
  }
  void refusalReasonsRemainActionableDuringRetries_data()
  {
    QTest::addColumn<int>("reason");
    QTest::addColumn<QString>("needle");
    QTest::newRow("duplicate") << int(ConnectionRefusal::AlreadyConnected) << "another client";
    QTest::newRow("unknown") << int(ConnectionRefusal::UnknownClient) << "does not recognize";
    QTest::newRow("version") << int(ConnectionRefusal::IncompatibleVersion) << "protocol version";
    QTest::newRow("protocol") << int(ConnectionRefusal::ProtocolError) << "protocol error";
  }
  void refusalReasonsRemainActionableDuringRetries()
  {
    QFETCH(int, reason);
    QFETCH(QString, needle);
    ConnectionDiagnostics d;
    start(d);
    d.connectionRefused(ConnectionRefusal(reason));
    d.retryIn(5);
    QCOMPARE(d.phase(), Phase::NeedsAction);
    QVERIFY(d.findChild<QLabel *>("diagnosticReason")->text().contains(needle, Qt::CaseInsensitive));
    QSignalSpy settings(&d, &ConnectionDiagnostics::settingsRequested);
    QSignalSpy reconnect(&d, &ConnectionDiagnostics::reconnectRequested);
    d.findChild<QPushButton *>("diagnosticAction")->click();
    QCOMPARE(settings.count(), 1);
    QCOMPARE(reconnect.count(), 0);
  }
  void retryButtonUsesExplicitActionAndDebounces()
  {
    ConnectionDiagnostics d;
    start(d);
    d.retryIn(5);
    QSignalSpy reconnect(&d, &ConnectionDiagnostics::reconnectRequested);
    auto *button = d.findChild<QPushButton *>("diagnosticAction");
    button->click();
    button->click();
    QCOMPARE(reconnect.count(), 1);
    QVERIFY(!button->isEnabled());
  }
  void serverListeningIsNotConnectedAndDoesNotInventPerPeerTls()
  {
    ConnectionDiagnostics d;
    d.beginAttempt(true, "", 24800, true, true);
    d.processStateChanged(ProcessState::Started);
    d.connectionStateChanged(ConnectionState::Listening);
    QCOMPARE(d.phase(), Phase::Listening);
    d.connectedClientsChanged({"Mac", "Laptop"});
    QCOMPARE(d.phase(), Phase::Connected);
    QVERIFY(d.findChild<QLabel *>("diagnosticReason")->text().contains("Per-client"));
    d.connectedClientsChanged({});
    QCOMPARE(d.phase(), Phase::Listening);
  }
  void discoveryNeverOverridesActiveConnectionOrGrantsTrust()
  {
    ConnectionDiagnostics d;
    d.setDiscoveryActive(true);
    QCOMPARE(d.phase(), Phase::Discovering);
    start(d);
    d.secureSocket(true);
    d.connectionStateChanged(ConnectionState::Connected);
    d.setDiscoveryActive(true);
    QCOMPARE(d.phase(), Phase::Connected);
  }
  void diagnosticsAreBoundedAndExcludeTargetOrContents()
  {
    ConnectionDiagnostics d;
    d.beginAttempt(false, "private-host-do-not-export", 24800, true, true);
    d.processStateChanged(ProcessState::Started);
    for (int i = 0; i < 90; ++i)
      d.retryIn(i);
    const auto snapshot = d.diagnosticSnapshot();
    QVERIFY(!snapshot.contains("private-host"));
    const auto data = QJsonDocument::fromJson(snapshot).object();
    QCOMPARE(data.value("events").toArray().size(), 64);
    QVERIFY(!data.value("latencyMeasured").toBool());
    QVERIFY(!data.value("inputDeliveryVerified").toBool());
    for (const auto &e : data.value("events").toArray())
      QCOMPARE(e.toObject().size(), 2);
  }
  void longPeerHasBoundedVisualWidthAndFullAccessibleText()
  {
    ConnectionDiagnostics d;
    d.resize(640, 300);
    d.show();
    QCoreApplication::processEvents();
    const QString target = QString(220, 'a') + ".invalid";
    d.beginAttempt(false, target, 25000, true, true);
    QCoreApplication::processEvents();
    auto *peer = d.findChild<QLabel *>("diagnosticPeer");
    QVERIFY(peer->accessibleName().contains(target));
    QVERIFY(peer->toolTip().contains("25000"));
    QVERIFY(peer->text().size() < target.size());
    QVERIFY(d.minimumSizeHint().width() < 700);
  }
};
QTEST_MAIN(ConnectionDiagnosticsTests)
#include "ConnectionDiagnosticsTests.moc"
