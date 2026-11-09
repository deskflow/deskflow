// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "../../deskflow/MockEventQueue.h"
#include "arch/Arch.h"
#include "base/Log.h"
#include "common/DesktopWorkspace.h"
#include "gui/IdentityBindings.h"
#include "gui/core/CoreProcess.h"
#include "gui/dialogs/TrustedDevicesDialog.h"
#include "gui/ipc/CoreIpcClient.h"
#include "net/SecureSocket.h"
#include "server/Config.h"
#include <QApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTextStream>
#include <sstream>

namespace deskflow::gui {
using namespace deskflow::auth;
class AuthenticationChainTests : public QObject
{
  Q_OBJECT
  const QString a = QString(64, 'a'), b = QString(64, 'b');
  Context context() const
  {
    return {QString(32, '1'), a, 1};
  }
  static void deliver()
  {
    QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
  }
  static ipc::CoreIpcClient *attach(CoreProcess &core)
  {
    auto *ipc = new ipc::CoreIpcClient(&core);
    core.m_coreIpcClient = ipc;
    core.bindCoreIpcClient(ipc);
    return ipc;
  }
  void seed(IdentityBindings &model)
  {
    auto g = model.begin({"192.168.50.20", 34800});
    model.observe(g, a);
    QVERIFY(model.confirm(g, QString::fromUtf8("工作电脑"), true));
    QVERIFY(model.authenticated(g, a, true, true));
  }
private Q_SLOTS:
  void workspaceRequiresCurrentVerifiedAttemptAndDoesNotCreateTrust()
  {
    using namespace deskflow::display;
    ServerConfig config(5, 3);
    CoreProcess core(config);
    IdentityBindings trust;
    const auto c = core.labBeginAuthentication(a);
    auto *ipc = attach(core);
    WorkspaceSnapshot snapshot;
    snapshot.devices = {"Work-PC", "Other-PC"};
    snapshot.onlineDevices = {"Work-PC"};
    const auto message = QString::fromUtf8(
        QJsonDocument(
            QJsonObject{{"device", "Work-PC"}, {"workspace", QJsonDocument::fromJson(snapshot.encode()).object()}}
        ).toJson(QJsonDocument::Compact)
    );
    QSignalSpy received(&core, &CoreProcess::workspaceSnapshotReceived);
    Q_EMIT ipc->commandReceived("workspaceSnapshot", message);
    deliver();
    QCOMPARE(received.count(), 0);
    Q_EMIT ipc->commandReceived("peerAuthentication", Report{c, 1, Outcome::Started, {}}.encode());
    Q_EMIT ipc->commandReceived("workspaceSnapshot", message);
    deliver();
    QCOMPARE(received.count(), 0);
    Q_EMIT ipc->commandReceived("peerAuthentication", Report{c, 1, Outcome::Verified, a}.encode());
    Q_EMIT ipc->commandReceived("workspaceSnapshot", message);
    deliver();
    QCOMPARE(received.count(), 1);
    QCOMPARE(received.first()[0].toString(), QString("Work-PC"));
    QCOMPARE(received.first()[2].toString(), a);
    QVERIFY(trust.devices().isEmpty());
    Q_EMIT ipc->commandReceived("connectionState", "Disconnected");
    Q_EMIT ipc->commandReceived("workspaceSnapshot", message);
    deliver();
    QCOMPARE(received.count(), 1);
    Q_EMIT ipc->commandReceived("peerAuthentication", Report{c, 2, Outcome::Started, {}}.encode());
    Q_EMIT ipc->commandReceived("peerAuthentication", Report{c, 2, Outcome::Verified, b}.encode());
    Q_EMIT ipc->commandReceived("workspaceSnapshot", message);
    deliver();
    QCOMPARE(received.count(), 1);
    const auto next = core.labBeginAuthentication(a);
    auto *fresh = attach(core);
    Q_EMIT fresh->commandReceived("peerAuthentication", Report{next, 1, Outcome::Started, {}}.encode());
    Q_EMIT fresh->commandReceived("peerAuthentication", Report{next, 1, Outcome::Verified, a}.encode());
    deliver();
    Q_EMIT ipc->commandReceived("workspaceSnapshot", message);
    deliver();
    QCOMPARE(received.count(), 1);
    Q_EMIT fresh->commandReceived("workspaceSnapshot", message);
    deliver();
    QCOMPARE(received.count(), 2);
    Q_EMIT fresh->commandReceived("workspaceSnapshot", message);
    core.stop(Settings::ProcessMode::Desktop);
    deliver();
    QCOMPARE(received.count(), 2);
  }
  void disconnectedAttemptCannotSupplyANameOrReviveWithLateVerification()
  {
    ServerConfig config(5, 3);
    CoreProcess core(config);
    const auto c = core.labBeginAuthentication(a);
    auto *ipc = attach(core);
    QSignalSpy names(&core, &CoreProcess::verifiedPeerNameReceived);
    const QString message = "{\"device\":\"Late-PC\",\"topology\":{}}";
    Q_EMIT ipc->commandReceived("peerAuthentication", Report{c, 1, Outcome::Started, {}}.encode());
    Q_EMIT ipc->commandReceived("peerAuthentication", Report{c, 1, Outcome::Verified, a}.encode());
    Q_EMIT ipc->commandReceived("connectionState", "Disconnected");
    Q_EMIT ipc->commandReceived("displayTopology", message);
    deliver();
    QCOMPARE(names.count(), 0);
    Q_EMIT ipc->commandReceived("peerAuthentication", Report{c, 2, Outcome::Started, {}}.encode());
    Q_EMIT ipc->commandReceived("connectionState", "Disconnected");
    Q_EMIT ipc->commandReceived("peerAuthentication", Report{c, 2, Outcome::Verified, a}.encode());
    Q_EMIT ipc->commandReceived("displayTopology", message);
    deliver();
    QCOMPARE(names.count(), 0);
    Q_EMIT ipc->commandReceived("peerAuthentication", Report{c, 3, Outcome::Started, {}}.encode());
    Q_EMIT ipc->commandReceived("peerAuthentication", Report{c, 3, Outcome::Verified, a}.encode());
    Q_EMIT ipc->commandReceived("displayTopology", message);
    deliver();
    QCOMPARE(names.count(), 1);
    QCOMPARE(names.first()[0].toString(), a);
  }
  void peerNameRequiresCurrentVerifiedIpcAndNeverCreatesTrust()
  {
    ServerConfig config(5, 3);
    CoreProcess core(config);
    IdentityBindings model;
    auto g = model.begin({"192.168.50.20", 34800});
    model.observe(g, a);
    QVERIFY(model.confirm(g, "192.168.50.20", true));
    QVERIFY(model.authenticated(g, a, true, true));
    connect(&core, &CoreProcess::verifiedPeerNameReceived, &core, [&](const QString &fp, const QString &name) {
      model.updateDefaultLabel(fp, name);
    });
    QSignalSpy names(&core, &CoreProcess::verifiedPeerNameReceived);
    auto c = core.labBeginAuthentication(a);
    auto *ipc = attach(core);
    const QString valid = "{\"device\":\"Work-PC\",\"topology\":{}}";
    Q_EMIT ipc->commandReceived("displayTopology", valid);
    deliver();
    QCOMPARE(names.count(), 0);
    Q_EMIT ipc->commandReceived("peerAuthentication", Report{c, 1, Outcome::Started, {}}.encode());
    Q_EMIT ipc->commandReceived("displayTopology", valid);
    deliver();
    QCOMPARE(names.count(), 0);
    Q_EMIT ipc->commandReceived("peerAuthentication", Report{c, 1, Outcome::Verified, a}.encode());
    Q_EMIT ipc->commandReceived("displayTopology", valid);
    deliver();
    QCOMPARE(names.count(), 1);
    QCOMPARE(names.first()[0].toString(), a);
    QCOMPARE(model.devices()[a].label, QString("Work-PC"));
    QCOMPARE(model.devices().size(), 1);
    Q_EMIT ipc->commandReceived("displayTopology", "{\"device\":\"<b>unsafe</b>\",\"topology\":{}}");
    Q_EMIT ipc->commandReceived("displayTopology", "{\"device\":\"Other\",\"topology\":{},\"trusted\":true}");
    deliver();
    QCOMPARE(names.count(), 1);
    Q_EMIT ipc->commandReceived("peerAuthentication", Report{c, 2, Outcome::Started, {}}.encode());
    Q_EMIT ipc->commandReceived("displayTopology", valid);
    deliver();
    QCOMPARE(names.count(), 1);
    Q_EMIT ipc->commandReceived("peerAuthentication", Report{c, 2, Outcome::WrongIdentity, b}.encode());
    Q_EMIT ipc->commandReceived("displayTopology", valid);
    deliver();
    QCOMPARE(names.count(), 1);
    c = core.labBeginAuthentication(a);
    auto *fresh = attach(core);
    Q_EMIT fresh->commandReceived("peerAuthentication", Report{c, 1, Outcome::Started, {}}.encode());
    Q_EMIT fresh->commandReceived("peerAuthentication", Report{c, 1, Outcome::Verified, a}.encode());
    deliver();
    Q_EMIT ipc->commandReceived("displayTopology", valid);
    deliver();
    QCOMPARE(names.count(), 1);
    Q_EMIT fresh->commandReceived("displayTopology", valid);
    core.stop(Settings::ProcessMode::Desktop);
    deliver();
    QCOMPARE(names.count(), 1);
  }
  void actualSecureSocketDecision_data()
  {
    QTest::addColumn<QString>("observed");
    QTest::addColumn<bool>("tls");
    QTest::addColumn<bool>("peers");
    QTest::addColumn<bool>("trusted");
    QTest::addColumn<int>("outcome");
    QTest::newRow("same-known-identity") << QString(64, 'a') << true << true << true << int(Outcome::Verified);
    QTest::newRow("different-but-in-trust-db")
        << QString(64, 'b') << true << true << true << int(Outcome::WrongIdentity);
    QTest::newRow("different-and-unknown") << QString(64, 'b') << true << true << false << int(Outcome::WrongIdentity);
    QTest::newRow("same-but-not-trusted") << QString(64, 'a') << true << true << false << int(Outcome::Untrusted);
    QTest::newRow("no-certificate") << QString() << true << true << true << int(Outcome::MissingCertificate);
    QTest::newRow("malformed-digest") << QString("AA:bad") << true << true << true << int(Outcome::MissingCertificate);
    QTest::newRow("no-tls") << QString(64, 'a') << false << true << true << int(Outcome::SecurityDisabled);
    QTest::newRow("peer-check-disabled") << QString(64, 'a') << true << false << true << int(Outcome::SecurityDisabled);
  }
  void actualSecureSocketDecision()
  {
    QFETCH(QString, observed);
    QFETCH(bool, tls);
    QFETCH(bool, peers);
    QFETCH(bool, trusted);
    QFETCH(int, outcome);
    const auto r =
        SecureSocket::authenticatePeerDigest({context(), 1, Outcome::Started, {}}, observed, tls, peers, trusted);
    QCOMPARE(int(r.outcome), outcome);
    QVERIFY(r.valid());
    auto wire = Report::decode(r.encode());
    QVERIFY(wire);
    QCOMPARE(wire->context, context());
    QCOMPARE(wire->outcome, r.outcome);
  }
  void codecRejectsMalformedAndInjectedFields()
  {
    const auto c = context();
    QCOMPARE(Context::decode(c.encode()), c);
    auto o = c.json();
    o["trust"] = true;
    QVERIFY(!Context::decode(pack(o)).valid());
    o = c.json();
    o["generation"] = 1;
    QVERIFY(!Context::decode(pack(o)).valid());
    o = c.json();
    o["generation"] = "18446744073709551616";
    QVERIFY(!Context::decode(pack(o)).valid());
    QVERIFY(!Context::decode(c.encode() + "=").valid());
    QVERIFY(!Context::decode(QString(4096, 'A')).valid());
    auto r = Report{c, 1, Outcome::Verified, a};
    o = unpack(r.encode());
    o["observed"] = b;
    QVERIFY(!Report::decode(pack(o)));
    o = unpack(r.encode());
    o["outcome"] = "trusted-from-discovery";
    QVERIFY(!Report::decode(pack(o)));
  }
  void sequenceMustStartAndCannotReplay()
  {
    Gate gate;
    auto c = context();
    QVERIFY(gate.begin(c));
    Report r{c, 1, Outcome::Verified, a};
    QVERIFY(!gate.consume(r));
    r.outcome = Outcome::Started;
    QVERIFY(gate.consume(r));
    r.outcome = Outcome::Verified;
    QVERIFY(gate.consume(r));
    QVERIFY(!gate.consume(r));
    r.sequence = 2;
    r.outcome = Outcome::Started;
    QVERIFY(gate.consume(r));
    r.sequence = 1;
    r.outcome = Outcome::Verified;
    QVERIFY(!gate.consume(r));
    r.sequence = 2;
    QVERIFY(gate.consume(r));
    gate.cancel();
    r.sequence = 3;
    r.outcome = Outcome::Started;
    QVERIFY(!gate.consume(r));
  }
  void rejectionCannotReviveFromAnotherSocket()
  {
    Gate gate;
    auto c = context();
    QVERIFY(gate.begin(c));
    Report r{c, 1, Outcome::Started, {}};
    QVERIFY(gate.consume(r));
    r.outcome = Outcome::WrongIdentity;
    r.observed = b;
    QVERIFY(gate.consume(r));
    r.sequence = 2;
    r.outcome = Outcome::Started;
    r.observed.clear();
    QVERIFY(!gate.consume(r));
  }
  void actualCoreIpcToIdentityPersistence()
  {
    ServerConfig config(5, 3);
    CoreProcess core(config);
    IdentityBindings model;
    seed(model);
    Settings::setValue(Settings::Client::RemoteHost, "192.168.50.30");
    Settings::setValue(Settings::Core::Port, 34800);
    connect(&core, &CoreProcess::authenticationCancelled, &core, [&] { model.cancel(); });
    connect(&core, &CoreProcess::authenticationStarting, &core, [&](const Context &c, const QString &host, int port) {
      QVERIFY(model.beginAuthentication(c, {host, port}));
    });
    bool saved = false;
    connect(&core, &CoreProcess::authenticationResult, &core, [&](const Report &r) {
      saved = model.consumeAuthentication(r) || saved;
    });
    const auto c = core.labBeginAuthentication(a);
    auto *ipc = attach(core);
    Q_EMIT ipc->commandReceived("peerAuthentication", Report{c, 1, Outcome::Started, {}}.encode());
    deliver();
    QCOMPARE(model.devices()[a].endpoint.host, QString("192.168.50.20"));
    auto proof = SecureSocket::authenticatePeerDigest({c, 1, Outcome::Started, {}}, a, true, true, true);
    Q_EMIT ipc->commandReceived("peerAuthentication", proof.encode());
    deliver();
    QVERIFY(saved);
    QCOMPARE(model.devices()[a].endpoint.host, QString("192.168.50.30"));
    IdentityBindings restored;
    QVERIFY(restored.load(model.encode()));
    QCOMPARE(restored.devices(), model.devices());
  }
  void wrongIdentityCannotRewriteAddressOrReportConnected()
  {
    ServerConfig config(5, 3);
    CoreProcess core(config);
    IdentityBindings model;
    seed(model);
    const auto c = core.labBeginAuthentication(a);
    QVERIFY(model.beginAuthentication(c, {"192.168.50.99", 34800}));
    connect(&core, &CoreProcess::authenticationResult, &core, [&](const Report &r) {
      QVERIFY(!model.consumeAuthentication(r));
    });
    auto *ipc = attach(core);
    Q_EMIT ipc->commandReceived("peerAuthentication", Report{c, 1, Outcome::Started, {}}.encode());
    deliver();
    auto r = SecureSocket::authenticatePeerDigest({c, 1, Outcome::Started, {}}, b, true, true, true);
    Q_EMIT ipc->commandReceived("peerAuthentication", r.encode());
    deliver();
    QVERIFY(core.m_recoveryBlocked);
    QVERIFY(!core.m_retryTimer.isActive());
    Q_EMIT ipc->commandReceived("connectionState", "Connected");
    Q_EMIT ipc->commandReceived("retryIn", "1");
    deliver();
    QCOMPARE(core.connectionState(), deskflow::core::ConnectionState::Disconnected);
    QCOMPARE(model.devices()[a].endpoint.host, QString("192.168.50.20"));
  }
  void oldResultAfterRestartAndStopIsRejected()
  {
    ServerConfig config(5, 3);
    CoreProcess core(config);
    auto old = core.labBeginAuthentication(a);
    auto *ipc = attach(core);
    QSignalSpy spy(&core, &CoreProcess::authenticationResult);
    Q_EMIT ipc->commandReceived("peerAuthentication", Report{old, 1, Outcome::Started, {}}.encode());
    auto current = core.labBeginAuthentication(a);
    auto *fresh = attach(core);
    deliver();
    QCOMPARE(spy.count(), 0);
    Q_EMIT fresh->commandReceived("peerAuthentication", Report{old, 1, Outcome::Verified, a}.encode());
    deliver();
    QCOMPARE(spy.count(), 0);
    Q_EMIT fresh->commandReceived("peerAuthentication", Report{current, 1, Outcome::Started, {}}.encode());
    deliver();
    QCOMPARE(spy.count(), 1);
    Q_EMIT fresh->commandReceived("peerAuthentication", Report{current, 1, Outcome::Verified, a}.encode());
    core.stop(Settings::ProcessMode::Desktop);
    deliver();
    QCOMPARE(spy.count(), 1);
    QVERIFY(!core.m_retryTimer.isActive());
  }
  void connectedRequiresCurrentAuthentication()
  {
    ServerConfig config(5, 3);
    CoreProcess core(config);
    auto c = core.labBeginAuthentication(a);
    auto *ipc = attach(core);
    Q_EMIT ipc->commandReceived("connectionState", "Connected");
    deliver();
    QVERIFY(core.connectionState() != deskflow::core::ConnectionState::Connected);
    Q_EMIT ipc->commandReceived("peerAuthentication", Report{c, 1, Outcome::Started, {}}.encode());
    Q_EMIT ipc->commandReceived("peerAuthentication", Report{c, 1, Outcome::Verified, a}.encode());
    Q_EMIT ipc->commandReceived("connectionState", "Connected");
    deliver();
    QCOMPARE(core.connectionState(), deskflow::core::ConnectionState::Connected);
  }
  void cancelledModelAndChangedSessionNeverCommit()
  {
    IdentityBindings model;
    seed(model);
    auto c = context();
    QVERIFY(model.beginAuthentication(c, {"192.168.50.30", 34800}));
    QVERIFY(!model.consumeAuthentication({c, 1, Outcome::Started, {}}));
    model.cancel();
    QVERIFY(!model.consumeAuthentication({c, 1, Outcome::Verified, a}));
    QVERIFY(model.beginAuthentication(c, {"192.168.50.30", 34800}));
    auto other = c;
    other.session = QString(32, '2');
    QVERIFY(!model.consumeAuthentication({other, 1, Outcome::Started, {}}));
    QVERIFY(!model.consumeAuthentication({other, 1, Outcome::Verified, a}));
    QCOMPARE(model.devices()[a].endpoint.host, QString("192.168.50.20"));
  }
  void explicitReviewStillNeedsBoundVerifiedProof()
  {
    IdentityBindings model;
    auto c = context();
    QVERIFY(model.beginReviewedAuthentication(c, {"192.168.50.30", 34800}, "Reviewed device"));
    QVERIFY(model.devices().isEmpty());
    QVERIFY(!model.consumeAuthentication({c, 1, Outcome::Verified, a}));
    QVERIFY(!model.consumeAuthentication({c, 1, Outcome::Started, {}}));
    QVERIFY(model.consumeAuthentication({c, 1, Outcome::Verified, a}));
    QCOMPARE(model.devices().size(), 1);
    IdentityBindings denied;
    QVERIFY(denied.beginReviewedAuthentication(c, {"192.168.50.30", 34800}, "Reviewed device"));
    QVERIFY(!denied.consumeAuthentication({c, 1, Outcome::Started, {}}));
    QVERIFY(!denied.consumeAuthentication({c, 1, Outcome::WrongIdentity, b}));
    QVERIFY(denied.devices().isEmpty());
  }
  void savedDeviceUiOnlyChangesMetadataAndCarriesIdentity()
  {
    IdentityBindings model;
    seed(model);
    TrustedDevicesDialog dialog(model, true);
    dialog.show();
    auto *list = dialog.findChild<QListWidget *>("trustedDeviceList");
    QCOMPARE(list->count(), 1);
    list->setCurrentRow(0);
    auto *name = dialog.findChild<QLineEdit *>("trustedDeviceName");
    name->setText(QString::fromUtf8("家里的电脑"));
    auto *rename = dialog.findChild<QPushButton *>("renameTrustedDevice");
    rename->setFocus();
    QTest::keyClick(rename, Qt::Key_Space);
    QCOMPARE(model.devices()[a].label, QString::fromUtf8("家里的电脑"));
    list->setCurrentRow(0);
    QSignalSpy request(&dialog, &TrustedDevicesDialog::connectRequested);
    dialog.findChild<QPushButton *>("connectTrustedDevice")->click();
    QCOMPARE(request.count(), 1);
    QCOMPARE(request.at(0).at(0).toString(), a);
    TrustedDevicesDialog active(model, false);
    auto *activeList = active.findChild<QListWidget *>("trustedDeviceList");
    activeList->setCurrentRow(0);
    QVERIFY(!active.findChild<QPushButton *>("connectTrustedDevice")->isEnabled());
    active.findChild<QPushButton *>("removeTrustedDevice")->click();
    QVERIFY(model.devices().isEmpty());
  }
  void wholeDesktopLayoutIsReadByExistingCore()
  {
    Settings::setValue(Settings::Core::ComputerName, "Mac");
    Settings::setValue(Settings::Core::Interface, QString()); // In-memory wildcard address; no DNS or bind.
    ServerConfig guiConfig(5, 3);
    guiConfig.addClient("Windows");
    QVERIFY(guiConfig.setDesktopLayout({{"Mac", -1440, 0, 1440, 900}, {"Windows", 0, 180, 1920, 1080}}));
    guiConfig.commit(); // Populate the existing core's Settings::knownComputers.
    QString text;
    QTextStream out(&text);
    out << guiConfig;
    out.flush();
    // Parse the actual GUI export with the existing core grammar. There are
    // no per-monitor fields or new protocol frames hidden behind this UI.
    MockEventQueue events;
    std::istringstream input(text.toStdString());
    deskflow::server::Config coreConfig(&events);
    try {
      input >> coreConfig;
    } catch (const std::exception &error) {
      QFAIL(error.what());
    }
    float target = 0;
    QCOMPARE(coreConfig.getNeighbor("Mac", Direction::Right, 0.6f, &target), std::string("Windows"));
    QVERIFY(qAbs(target - 1.0f / 3) < 0.002f);
    float roundTrip = 0;
    QCOMPARE(coreConfig.getNeighbor("Windows", Direction::Left, target, &roundTrip), std::string("Mac"));
    QVERIFY(qAbs(roundTrip - 0.6f) < 0.002f);
    QVERIFY(coreConfig.getNeighbor("Mac", Direction::Right, 0.1f, &target).empty());
    QVERIFY(!text.contains("monitor"));
  }
};
} // namespace deskflow::gui
int main(int argc, char **argv)
{
  auto root = qEnvironmentVariable("DESKFLOW_TEST_ROOT");
  if (root.isEmpty() || !QDir(root).isAbsolute() || !QDir(root).exists())
    return 2;
  QTemporaryDir fixture(root + "/auth-chain-XXXXXX");
  if (!fixture.isValid())
    return 2;
  qputenv("XDG_CONFIG_HOME", (fixture.path() + "/config").toUtf8());
  qputenv("XDG_STATE_HOME", (fixture.path() + "/state").toUtf8());
  QDir().mkpath(fixture.path() + "/config/DeskflowLab");
  QDir().mkpath(fixture.path() + "/state");
  QApplication app(argc, argv);
  Arch arch;
#ifdef Q_OS_WIN
  // Initialize the same Winsock dispatch used by the real Windows core before
  // parsing its in-memory wildcard address. This opens no socket or listener.
  arch.init();
#endif
  Log log;
  Settings::setValue(Settings::Core::ComputerName, "SyntheticLab");
  Settings::setValue(Settings::Core::ProcessMode, Settings::ProcessMode::Desktop);
  deskflow::gui::AuthenticationChainTests tests;
  return QTest::qExec(&tests, argc, argv);
}
#include "AuthenticationChainTests.moc"
