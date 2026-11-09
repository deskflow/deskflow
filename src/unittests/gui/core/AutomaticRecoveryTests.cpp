// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "gui/core/AutomaticRecovery.h"
#include "gui/core/DiscoveryTransport.h"
#include "gui/core/NearbyDiscovery.h"
#include "gui/core/NetworkMonitor.h"
#include "gui/core/PowerMonitor.h"
#include "gui/dialogs/AutomaticRecoveryDialog.h"
#include "gui/ipc/CoreIpcClient.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QPushButton>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
namespace deskflow::gui {
class AutoTransport final : public DiscoveryTransport
{
public:
  int starts = 0;
  DiscoveryScope selected;
  AutoTransport() : DiscoveryTransport(nullptr)
  {
  }
  void browse(const DiscoveryScope &s) override
  {
    ++starts;
    selected = s;
    Q_EMIT activeChanged(true);
  }
  void advertise(const DiscoveryAdvertisement &) override
  {
  }
  void stop() override
  {
    Q_EMIT activeChanged(false);
  }
};
class AutoResolver final : public ScopedAddressResolver
{
public:
  int starts = 0;
  void start(const deskflow::auth::Context &c, const DiscoveryScope &s, const QString &h, int) override
  {
    ++starts;
    Q_EMIT addresses(c, s, h, {"192.168.50.30"}, 10000);
  }
  void cancel() override
  {
  }
};
class AutoProbe final : public TlsProbeExecutor
{
public:
  int starts = 0;
  int port = 34800;
  bool available = true;
  deskflow::auth::Context context;
  QString numeric;
  bool ready() const override
  {
    return available;
  }
  bool permitsPort(int value) const override
  {
    return port == value;
  }
  bool selectPreparedSession(const QString &) override
  {
    return false;
  }
  void start(const deskflow::auth::Context &c, const DiscoveryScope &, const QString &n, int) override
  {
    ++starts;
    context = c;
    numeric = n;
  }
  void cancel() override
  {
  }
  void complete(const QString &pin)
  {
    Q_EMIT completed(context, numeric, {context, 1, deskflow::auth::Outcome::Verified, pin});
  }
};
class AutomaticRecoveryTests : public QObject
{
  Q_OBJECT
  using State = AutomaticRecovery::State;
  using Process = deskflow::core::ProcessState;
  using Connection = deskflow::core::ConnectionState;
  struct Fixture
  {
    const QString pin = QString(64, 'a');
    DiscoveryScope scope{7, "Lab-LAN", QHostAddress("192.168.50.10"), 24};
    ServerConfig config{5, 3};
    CoreProcess core{config};
    IdentityBindings bindings;
    AutoTransport transport;
    NearbyDiscovery discovery{nullptr, &transport};
    AutoResolver resolver;
    AutoProbe probe;
    qint64 now = 0;
    QVector<DiscoveryScope> scopes{scope};
    AutomaticRecovery automatic{
        core, bindings, discovery, resolver, probe, [this] {return scopes; }, nullptr, [this] {return now; }
    };
    ipc::CoreIpcClient *client = nullptr;
    Fixture()
    {
      auto g = bindings.begin({"old.local", 34800});
      bindings.observe(g, pin);
      bindings.confirm(g, "Known", true);
      bindings.authenticated(g, pin, true, true);
      core.m_mode = Settings::CoreMode::Client;
      core.m_startRequested = true;
      core.m_expectedPeer = pin;
      core.m_connectionState = Connection::Connected;
      core.m_processState = Process::Started;
      client = new ipc::CoreIpcClient(&core);
      core.m_coreIpcClient = client;
      core.bindCoreIpcClient(client);
      if (!automatic.configure({true, pin, scope}))
        qFatal("invalid synthetic fixture");
    }
    ~Fixture()
    {
      automatic.cancel();
      core.stop(Settings::ProcessMode::Desktop);
    }
    void flush()
    {
      for (int i = 0; i < 4; ++i)
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
    }
    void disconnect()
    {
      Q_EMIT client->commandReceived("connectionState", "Disconnected");
      flush();
    }
    void search()
    {
      disconnect();
      now += 5000;
      automatic.tick();
      flush();
    }
    void candidate()
    {
      Q_EMIT transport.resolved("candidate", "Untrusted name", "new.local", 34800, 120000);
      flush();
    }
  };
private Q_SLOTS:
  void init()
  {
    Settings::setValue(Settings::Core::ProcessMode, Settings::ProcessMode::Desktop);
    Settings::setValue(Settings::Security::TlsEnabled, true);
    Settings::setValue(Settings::Security::CheckPeers, true);
    Settings::setValue(Settings::Client::RemoteHost, "old.local");
    Settings::setValue(Settings::Core::Port, 34800);
  }
  void inactiveRecoveryMustNotCancelForeignIdentityReview()
  {
    Fixture f;
    QVERIFY(f.automatic.configure({}));
    auto g = f.bindings.begin({"manual.local", 34800});
    f.bindings.observe(g, QString(64, 'b'));
    f.automatic.cancel();
    QCOMPARE(f.bindings.attempt().generation, g);
    QCOMPARE(f.bindings.attempt().review, IdentityBindings::Review::VerifyNew);
    Q_EMIT f.core.authenticationResult(
        {{QString(32, 'e'), f.pin, 7}, 1, deskflow::auth::Outcome::WrongIdentity, QString(64, 'b')}
    );
    QCOMPARE(f.bindings.attempt().generation, g);
    QCOMPARE(f.automatic.state(), State::Disabled);
  }
  void activeRecoveryMustNotCancelNewerForeignAuthentication()
  {
    Fixture f;
    f.search();
    f.candidate();
    auto g = f.bindings.begin({"manual.local", 34800});
    f.bindings.observe(g, QString(64, 'b'));
    f.automatic.cancel();
    QCOMPARE(f.bindings.attempt().generation, g);
    QCOMPARE(f.bindings.attempt().review, IdentityBindings::Review::VerifyNew);
  }
  void loginStartRestoresOnlySavedMatchingIdentity()
  {
    Fixture f;
    f.core.stop(Settings::ProcessMode::Desktop);
    f.core.m_expectedPeer.clear();
    QVERIFY(!f.automatic.prepareStart("different.local", 34800));
    QVERIFY(f.core.expectedPeerFingerprint().isEmpty());
    QVERIFY(!f.automatic.prepareStart("old.local", 24800));
    QVERIFY(f.automatic.prepareStart("old.local", 34800));
    QCOMPARE(f.core.expectedPeerFingerprint(), f.pin);
  }
  void queuedDisconnectProofResumeIsOneTransaction()
  {
    Fixture f;
    QSignalSpy resumed(&f.core, &CoreProcess::recoveredConnectionRequested),
        blocked(&f.core, &CoreProcess::inputExecutionUnavailable);
    f.disconnect();
    QCOMPARE(f.automatic.state(), State::Debouncing);
    f.now = 4999;
    f.automatic.tick();
    QCOMPARE(f.transport.starts, 0);
    f.now = 5000;
    f.automatic.tick();
    f.flush();
    QCOMPARE(f.automatic.state(), State::Searching);
    QVERIFY(f.core.recoveryQuiescent());
    f.candidate();
    QCOMPARE(f.probe.starts, 1);
    QCOMPARE(f.bindings.devices()[f.pin].endpoint.host, QString("old.local"));
    f.probe.complete(f.pin);
    QCOMPARE(resumed.count(), 1);
    QCOMPARE(blocked.count(), 1);
    QCOMPARE(f.bindings.devices()[f.pin].endpoint.host, QString("192.168.50.30"));
    QCOMPARE(Settings::value(Settings::Client::RemoteHost).toString(), QString("192.168.50.30"));
    QCOMPARE(f.core.expectedPeerFingerprint(), f.pin);
    QVERIFY(!f.core.m_process);
    QCOMPARE(f.automatic.state(), State::NeedsAction);
    f.probe.complete(f.pin);
    QCOMPARE(resumed.count(), 1);
  }
  void configuredCandidatePortFlowsThroughActualBroker()
  {
    Fixture f;
    f.probe.port = 24800;
    f.search();
    Q_EMIT f.transport.resolved("candidate", "Untrusted name", "new.local", 24800, 120000);
    f.flush();
    QCOMPARE(f.probe.starts, 1);
    f.probe.complete(f.pin);
    QCOMPARE(f.bindings.devices()[f.pin].endpoint.port, 24800);
    QCOMPARE(Settings::value(Settings::Core::Port).toInt(), 24800);
    QVERIFY(!f.core.m_process);
  }
  void heldQuiescentCoreAllowsExplicitSavedDeviceSelection()
  {
    Fixture f;
    QVERIFY(!f.core.setExpectedPeerFingerprint(f.pin));
    f.search();
    QVERIFY(f.core.recoveryQuiescent());
    QVERIFY(f.core.setExpectedPeerFingerprint(f.pin));
    QVERIFY(!f.core.setExpectedPeerFingerprint("invalid"));
  }
  void explicitStopBeforeDebounceDoesNotSearch()
  {
    Fixture f;
    f.disconnect();
    f.core.stop(Settings::ProcessMode::Desktop);
    f.now = 10000;
    f.automatic.tick();
    f.automatic.networkChanged();
    QCOMPARE(f.transport.starts, 0);
    QCOMPARE(f.automatic.state(), State::Paused);
    QVERIFY(!f.core.connectionRequested());
  }
  void stopDuringProbeRejectsLateResult()
  {
    Fixture f;
    f.search();
    f.candidate();
    QSignalSpy resumed(&f.core, &CoreProcess::recoveredConnectionRequested);
    f.core.stop(Settings::ProcessMode::Desktop);
    f.probe.complete(f.pin);
    f.flush();
    QCOMPARE(resumed.count(), 0);
    QCOMPARE(f.bindings.devices()[f.pin].endpoint.host, QString("old.local"));
    QCOMPARE(f.automatic.state(), State::Paused);
  }
  void wakeAndNetworkBurstDebounceOneRound()
  {
    Fixture f;
    f.disconnect();
    PowerMonitor power;
    NetworkMonitor network;
    connect(&power, &PowerMonitor::sleepingChanged, &f.automatic, &AutomaticRecovery::powerChanged);
    connect(&network, &NetworkMonitor::networkConfigurationChanged, &f.automatic, &AutomaticRecovery::networkChanged);
    power.observe(true);
    power.observe(true);
    f.now = 10000;
    f.automatic.tick();
    QCOMPARE(f.transport.starts, 0);
    power.observe(false);
    power.observe(false);
    Q_EMIT network.networkConfigurationChanged();
    Q_EMIT network.networkConfigurationChanged();
    f.now = 11499;
    f.automatic.tick();
    QCOMPARE(f.transport.starts, 0);
    f.now = 11500;
    f.automatic.tick();
    f.flush();
    QCOMPARE(f.transport.starts, 1);
  }
  void longSleepDoesNotConsumeAwakeRecoveryBudget()
  {
    Fixture f;
    f.disconnect();
    f.now = 1000;
    f.automatic.powerChanged(true);
    f.now = 3601000;
    f.automatic.tick();
    f.automatic.powerChanged(false);
    f.now += 1500;
    f.automatic.tick();
    f.flush();
    QCOMPARE(f.transport.starts, 1);
  }
  void stoppedUserStaysPausedAcrossPowerAndNetwork()
  {
    Fixture f;
    f.core.stop(Settings::ProcessMode::Desktop);
    f.automatic.powerChanged(true);
    f.automatic.powerChanged(false);
    f.automatic.networkChanged();
    f.now = 90000;
    f.automatic.tick();
    QCOMPARE(f.transport.starts, 0);
    QCOMPARE(f.automatic.state(), State::Paused);
  }
  void oldGenerationPowerAndDisconnectAreIgnored()
  {
    Fixture f;
    QSignalSpy power(&f.core, &CoreProcess::powerStateChanged);
    Q_EMIT f.client->commandReceived("powerState", "sleep");
    Q_EMIT f.client->commandReceived("connectionState", "Disconnected");
    f.core.invalidatePendingEvents();
    f.flush();
    QCOMPARE(power.count(), 0);
    QCOMPARE(f.core.connectionState(), Connection::Connected);
    QCOMPARE(f.transport.starts, 0);
  }
  void currentPowerMessagesAndUnknownValues()
  {
    Fixture f;
    QSignalSpy power(&f.core, &CoreProcess::powerStateChanged);
    Q_EMIT f.client->commandReceived("powerState", "sleep");
    Q_EMIT f.client->commandReceived("powerState", "invalid");
    Q_EMIT f.client->commandReceived("powerState", "wake");
    f.flush();
    QCOMPARE(power.count(), 2);
  }
  void sameInterfaceLocalDhcpChangeDoesNotChangeIdentity()
  {
    Fixture f;
    f.scopes[0].address = QHostAddress("192.168.50.11");
    f.search();
    QCOMPARE(f.transport.starts, 1);
    QCOMPARE(f.transport.selected.address, f.scopes[0].address);
    QCOMPARE(f.core.expectedPeerFingerprint(), f.pin);
  }
  void unapprovedNetwork_data()
  {
    QTest::addColumn<int>("variant");
    for (int i = 0; i < 5; ++i)
      QTest::newRow(qPrintable(QString::number(i))) << i;
  }
  void unapprovedNetwork()
  {
    QFETCH(int, variant);
    Fixture f;
    if (variant == 0)
      f.scopes.clear();
    if (variant == 1)
      f.scopes[0].index = 8;
    if (variant == 2)
      f.scopes[0].name = "Other-LAN";
    if (variant == 3)
      f.scopes[0].address = QHostAddress("192.168.51.10");
    if (variant == 4)
      f.scopes[0].prefix = 16;
    f.search();
    QCOMPARE(f.transport.starts, 0);
    QCOMPARE(f.automatic.state(), State::WaitingNetwork);
    f.scopes = {f.scope};
    f.automatic.networkChanged();
    f.now += 1500;
    f.automatic.tick();
    f.flush();
    QCOMPARE(f.transport.starts, 1);
  }
  void missingCredentialsDoesNotStopOriginalCore()
  {
    Fixture f;
    f.probe.available = false;
    f.search();
    QCOMPARE(f.automatic.state(), State::NeedsAction);
    QCOMPARE(f.core.processState(), Process::Started);
    QVERIFY(!f.core.m_addressRecoveryHold);
    QCOMPARE(f.transport.starts, 0);
  }
  void wrongIdentityNeverCommits()
  {
    Fixture f;
    f.search();
    f.candidate();
    QSignalSpy resumed(&f.core, &CoreProcess::recoveredConnectionRequested);
    f.probe.complete(QString(64, 'b'));
    QCOMPARE(resumed.count(), 0);
    QCOMPARE(f.bindings.devices()[f.pin].endpoint.host, QString("old.local"));
  }
  void refusedConfigurationBlocksDiscovery_data()
  {
    QTest::addColumn<int>("variant");
    for (int i = 0; i < 5; ++i)
      QTest::newRow(qPrintable(QString::number(i))) << i;
  }
  void refusedConfigurationBlocksDiscovery()
  {
    QFETCH(int, variant);
    Fixture f;
    if (variant == 0)
      Settings::setValue(Settings::Security::TlsEnabled, false);
    if (variant == 1)
      Settings::setValue(Settings::Security::CheckPeers, false);
    if (variant == 2)
      Settings::setValue(Settings::Core::ProcessMode, Settings::ProcessMode::Service);
    if (variant == 3)
      f.core.m_expectedPeer = QString(64, 'b');
    if (variant == 4)
      f.core.m_mode = Settings::CoreMode::Server;
    f.disconnect();
    f.now = 10000;
    f.automatic.tick();
    QCOMPARE(f.transport.starts, 0);
  }
  void terminalCoreFailureNeedsUserAction()
  {
    Fixture f;
    f.disconnect();
    Q_EMIT f.core.error(CoreProcess::Error::StartFailed);
    f.now = 6000;
    f.automatic.tick();
    QCOMPARE(f.transport.starts, 0);
    f.automatic.networkChanged();
    QCOMPARE(f.transport.starts, 0);
  }
  void candidateCancelledByNetworkRejectsOldProof()
  {
    Fixture f;
    f.search();
    f.candidate();
    auto old = f.probe.context;
    f.automatic.networkChanged();
    f.probe.complete(f.pin);
    QCOMPARE(f.bindings.devices()[f.pin].endpoint.host, QString("old.local"));
    f.now += 1500;
    f.automatic.tick();
    f.flush();
    f.candidate();
    QVERIFY(f.probe.context != old);
  }
  void repeatedInterruptionsHaveThreeRoundLimit()
  {
    Fixture f;
    f.search();
    for (int i = 0; i < 2; ++i) {
      f.automatic.networkChanged();
      f.now += 1500;
      f.automatic.tick();
      f.flush();
    }
    QCOMPARE(f.transport.starts, 3);
    f.automatic.networkChanged();
    f.now += 1500;
    f.automatic.tick();
    QCOMPARE(f.transport.starts, 3);
    f.automatic.networkChanged();
    QCOMPARE(f.transport.starts, 3);
  }
  void outageDeadlineBoundsMissingNetwork()
  {
    Fixture f;
    f.scopes.clear();
    f.search();
    f.now = 120000;
    f.automatic.tick();
    QCOMPARE(f.automatic.state(), State::NeedsAction);
    f.scopes = {f.scope};
    f.automatic.networkChanged();
    f.now += 2000;
    f.automatic.tick();
    QCOMPARE(f.transport.starts, 0);
  }
  void stoppedStateAloneDoesNotAllowDuplicateProcess()
  {
    Fixture f;
    QProcess owned;
    owned.start(QCoreApplication::applicationFilePath(), {"--idle-core-fixture"});
    QVERIFY(owned.waitForStarted(2000));
    f.disconnect();
    f.core.m_process = &owned;
    f.core.m_processState = Process::Stopped;
    const auto quiescent = f.core.recoveryQuiescent();
    f.now = 5000;
    f.automatic.tick();
    const auto before = f.automatic.state();
    f.now += 5000;
    f.automatic.tick();
    const auto after = f.automatic.state();
    owned.kill();
    const bool finished = owned.waitForFinished(2000);
    f.core.m_process = nullptr;
    QVERIFY(finished);
    QVERIFY(!quiescent);
    QCOMPARE(before, State::Stopping);
    QCOMPARE(after, State::NeedsAction);
    QCOMPARE(f.transport.starts, 0);
  }
  void policyCannotGrantDifferentIdentityAndCorruptDataFallsBackDisabled()
  {
    Fixture f;
    const AutomaticRecoveryPolicy p{true, f.pin, f.scope};
    QCOMPARE(AutomaticRecoveryPolicy::decode(p.encode()).encode(), p.encode());
    QVERIFY(!AutomaticRecoveryPolicy::decode(p.encode() + "junk").enabled);
    QVERIFY(!AutomaticRecoveryPolicy::decode(QByteArray(2049, 'x')).enabled);
    auto wrong = p;
    wrong.fingerprint = QString(64, 'b');
    QVERIFY(!f.automatic.configure(wrong));
    QCOMPARE(f.automatic.policy().fingerprint, f.pin);
  }
  void formRequiresExplicitNetworkAndCancelDoesNotApply()
  {
    Fixture f;
    AutomaticRecoveryDialog d(f.pin, {}, [&] { return f.scopes; });
    d.show();
    QTest::qWait(1);
    auto *enabled = d.findChild<QCheckBox *>("automaticRecoveryEnabled");
    auto *network = d.findChild<QComboBox *>("automaticRecoveryNetwork");
    auto *buttons = d.findChild<QDialogButtonBox *>();
    QVERIFY(enabled && network && buttons);
    enabled->setChecked(true);
    QVERIFY(!buttons->button(QDialogButtonBox::Save)->isEnabled());
    network->setCurrentIndex(1);
    QVERIFY(buttons->button(QDialogButtonBox::Save)->isEnabled());
    QVERIFY(d.policy().valid());
    QTest::keyClick(&d, Qt::Key_Escape);
    QCOMPARE(d.result(), int(QDialog::Rejected));
    QCOMPARE(f.automatic.policy().fingerprint, f.pin);
  }
};
} // namespace deskflow::gui
int main(int argc, char **argv)
{
  if (argc == 2 && QByteArray(argv[1]) == "--idle-core-fixture") {
    QCoreApplication app(argc, argv);
    QTimer::singleShot(10000, &app, &QCoreApplication::quit);
    return app.exec();
  }
  const auto root = qEnvironmentVariable("DESKFLOW_TEST_ROOT");
  if (root.isEmpty() || !QDir(root).isAbsolute() || !QDir(root).exists())
    return 2;
  QTemporaryDir fixture(root + "/automatic-XXXXXX");
  if (!fixture.isValid())
    return 2;
  qputenv("XDG_CONFIG_HOME", (fixture.path() + "/config").toUtf8());
  qputenv("XDG_STATE_HOME", (fixture.path() + "/state").toUtf8());
  QDir().mkpath(fixture.path() + "/config/DeskflowLab");
  QDir().mkpath(fixture.path() + "/state");
  QApplication app(argc, argv);
  app.setStyle("Fusion");
  Settings::setValue(Settings::Core::ComputerName, "SyntheticLab");
  deskflow::gui::AutomaticRecoveryTests tests;
  return QTest::qExec(&tests, argc, argv);
}
#include "AutomaticRecoveryTests.moc"
