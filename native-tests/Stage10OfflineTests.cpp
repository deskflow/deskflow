// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "common/IsolatedSession.h"
#include "common/Settings.h"
#include "gui/IdentityBindings.h"
#include "gui/config/MonitorTopology.h"
#include "gui/dialogs/DiagnosticExportDialog.h"
#include "gui/dialogs/DisplayTopologyDialog.h"
#include "gui/dialogs/PairingReviewDialog.h"
#include "gui/dialogs/TrustedDevicesDialog.h"
#include "gui/widgets/ConnectionDiagnostics.h"
#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTranslator>
#include <QUuid>
#include <limits>
using deskflow::gui::IdentityBindings;
using deskflow::gui::MonitorTopology;
using deskflow::lab::IsolatedSession;
class Stage10OfflineTests : public QObject
{
  Q_OBJECT
  const QString a = QString(64, 'a'), b = QString(64, 'b'), c = QString(64, 'c');
  const IdentityBindings::Endpoint oldAddress{"192.168.50.20", 34800}, newAddress{"192.168.50.30", 34800};
  QJsonObject spec(const QString &id)
  {
    return {
        {"schema", 1},
        {"purpose", "deskflow-lab-network"},
        {"session", id},
        {"mode", "client"},
        {"bindAddress", "127.0.0.1"},
        {"peerAddress", "192.168.50.20"},
        {"port", 34800},
        {"tls", true},
        {"peerChecks", true}
    };
  }
  QString fixture(const QString &parent, const QJsonObject &o = {})
  {
    const auto id = QUuid::createUuid().toString(QUuid::Id128);
    const auto path = parent + '/' + id;
    for (const auto *name : {"config/DeskflowLab/tls", "state", "runtime", "logs"})
      QDir().mkpath(path + '/' + name);
    QFile f(path + "/session.json");
    if (!f.open(QIODevice::WriteOnly))
      return {};
    f.write(QJsonDocument(o.isEmpty() ? spec(id) : o).toJson());
    return path;
  }
  void rewrite(const QString &path, const QJsonObject &o)
  {
    QFile f(path + "/session.json");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(QJsonDocument(o).toJson());
  }
  void seed(IdentityBindings &s)
  {
    const auto g = s.begin(oldAddress);
    QVERIFY(g);
    QCOMPARE(s.observe(g, a), IdentityBindings::Review::VerifyNew);
    QVERIFY(s.confirm(g, QString::fromUtf8("工作电脑"), true));
    QVERIFY(s.authenticated(g, a, true, true));
  }
private Q_SLOTS:
  void freshCorePlanIsReadOnlyAndFullyNamespaced()
  {
    QTemporaryDir temp;
    const auto parent = QFileInfo(temp.path()).canonicalFilePath();
    const auto path = fixture(parent);
    const auto s = IsolatedSession::inspect(path, parent);
    QVERIFY2(s.valid(), qPrintable(s.error));
    QCOMPARE(s.port, 34800);
    QVERIFY(s.settings.startsWith(path + "/"));
    QVERIFY(s.state.startsWith(path + "/"));
    QVERIFY(s.identity.startsWith(path + "/config/DeskflowLab/tls/"));
    QVERIFY(s.trustedServers.startsWith(path + "/"));
    QVERIFY(s.ipcName.startsWith("deskflow-lab-ipc-"));
    QVERIFY(s.lockFile.startsWith(path + "/runtime/"));
    QVERIFY(!QFileInfo::exists(s.settings));
    QVERIFY(!QFileInfo::exists(s.identity));
    QVERIFY(!QFileInfo::exists(s.lockFile));
    const auto plan = s.plan();
    QCOMPARE(plan["executionAvailable"], false);
    QCOMPARE(plan["networkExecuted"], false);
    QCOMPARE(plan["ipcCreated"], false);
    const auto another = IsolatedSession::inspect(fixture(parent), parent);
    QVERIFY(another.valid());
    QVERIFY(s.ipcName != another.ipcName);
  }
  void rejectProductionAndTraversalRoots_data()
  {
    QTest::addColumn<QString>("path");
    QTest::newRow("production-app") << "/Applications/Deskflow.app";
    QTest::newRow("production-config") << "/Users/test/Library/Preferences/Deskflow";
    QTest::newRow("relative") << "relative";
    QTest::newRow("empty") << "";
    QTest::newRow("traversal") << "/tmp/../Applications/Deskflow.app";
  }
  void rejectProductionAndTraversalRoots()
  {
    QFETCH(QString, path);
    QTemporaryDir temp;
    QVERIFY(!IsolatedSession::inspect(path, QFileInfo(temp.path()).canonicalFilePath()).valid());
  }
  void rejectUnsafeManifest_data()
  {
    QTest::addColumn<QString>("key");
    QTest::addColumn<QJsonValue>("value");
    QTest::newRow("production-port") << "port" << QJsonValue(24800);
    QTest::newRow("arbitrary-port") << "port" << QJsonValue(443);
    QTest::newRow("tls-off") << "tls" << QJsonValue(false);
    QTest::newRow("pins-off") << "peerChecks" << QJsonValue(false);
    QTest::newRow("wildcard") << "bindAddress" << QJsonValue("0.0.0.0");
    QTest::newRow("public") << "peerAddress" << QJsonValue("8.8.8.8");
    QTest::newRow("hostname") << "peerAddress" << QJsonValue("example.org");
    QTest::newRow("service-mode") << "mode" << QJsonValue("service");
    QTest::newRow("config-override") << "settingsPath" << QJsonValue("/Applications/Deskflow.app/config");
    QTest::newRow("ipc-override") << "ipcName" << QJsonValue("deskflow-core");
    QTest::newRow("certificate-override") << "identityPath" << QJsonValue("/production.pem");
    QTest::newRow("wrong-session") << "session" << QJsonValue("different");
  }
  void rejectUnsafeManifest()
  {
    QFETCH(QString, key);
    QFETCH(QJsonValue, value);
    QTemporaryDir temp;
    const auto parent = QFileInfo(temp.path()).canonicalFilePath();
    const auto path = fixture(parent);
    auto o = spec(QFileInfo(path).fileName());
    o[key] = value;
    rewrite(path, o);
    QVERIFY(!IsolatedSession::inspect(path, parent).valid());
  }
  void refuseSymlinkAndExistingCredentialWithoutReadingIt()
  {
    QTemporaryDir temp;
    const auto parent = QFileInfo(temp.path()).canonicalFilePath();
    const auto path = fixture(parent);
    QFile marker(parent + "/untouched");
    QVERIFY(marker.open(QIODevice::WriteOnly));
    marker.write("test-sentinel");
    marker.close();
    const auto key = path + "/config/DeskflowLab/tls/test-identity.pem";
    QVERIFY(QFile::link(marker.fileName(), key));
    QCOMPARE(IsolatedSession::inspect(path, parent).error, "fresh-session-files-required");
    QVERIFY(marker.open(QIODevice::ReadOnly));
    QCOMPARE(marker.readAll(), QByteArray("test-sentinel"));
  }
  void refuseRedirectedDirectoryAndOversizedManifest()
  {
    QTemporaryDir temp;
    const auto parent = QFileInfo(temp.path()).canonicalFilePath();
    auto path = fixture(parent);
    QVERIFY(QDir(path + "/state").removeRecursively());
    QVERIFY(QFile::link(parent, path + "/state"));
    QCOMPARE(IsolatedSession::inspect(path, parent).error, "missing-or-redirected-session-directory");
    path = fixture(parent);
    QFile f(path + "/session.json");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(QByteArray(5000, 'x'));
    f.close();
    QCOMPARE(IsolatedSession::inspect(path, parent).error, "invalid-session-manifest");
  }
  void discoveryDoesNotCreateIdentityOrPersistUnverifiedAddress()
  {
    IdentityBindings s;
    const auto g = s.begin(oldAddress);
    QVERIFY(g);
    QVERIFY(s.devices().isEmpty());
    s.observe(g, a);
    QVERIFY(s.devices().isEmpty());
    QVERIFY(!s.confirm(g, "Device", false));
    QVERIFY(!s.authenticated(g, a, true, true));
    QVERIFY(s.devices().isEmpty());
    QVERIFY(s.confirm(g, "Device", true));
    QVERIFY(s.devices().isEmpty());
    QVERIFY(!s.authenticated(g, a, false, true));
    QVERIFY(!s.authenticated(g, a, true, false));
    QVERIFY(s.authenticated(g, a, true, true));
    QCOMPARE(s.devices().size(), 1);
  }
  void sameIdentityMovesAddressOnlyAfterVerifiedTls()
  {
    IdentityBindings s;
    seed(s);
    const auto g = s.begin(newAddress, a);
    QCOMPARE(s.observe(g, a), IdentityBindings::Review::KnownIdentity);
    QCOMPARE(s.devices()[a].endpoint, oldAddress);
    QVERIFY(!s.authenticated(g, b, true, true));
    QCOMPARE(s.devices()[a].endpoint, oldAddress);
    QVERIFY(s.authenticated(g, a, true, true));
    QCOMPARE(s.devices()[a].endpoint, newAddress);
  }
  void changedCertificateNeverRebindsEvenIfSeparatelyKnown()
  {
    IdentityBindings s;
    seed(s);
    auto g = s.begin({"other.local", 34800});
    s.observe(g, b);
    s.confirm(g, "Other", true);
    QVERIFY(s.authenticated(g, b, true, true));
    g = s.begin(newAddress, a);
    QCOMPARE(s.observe(g, b), IdentityBindings::Review::ChangedIdentity);
    QVERIFY(!s.confirm(g, "Changed", true));
    QVERIFY(!s.authenticated(g, b, true, true));
    QCOMPARE(s.devices()[a].endpoint, oldAddress);
  }
  void staleCancelForgetAndRepeatedCertificates()
  {
    IdentityBindings s;
    auto g = s.begin(oldAddress);
    s.observe(g, a);
    s.confirm(g, "Device", true);
    s.cancel();
    QVERIFY(!s.authenticated(g, a, true, true));
    QVERIFY(s.devices().isEmpty());
    seed(s);
    g = s.begin(newAddress, a);
    s.observe(g, a);
    QVERIFY(s.forget(a));
    QVERIFY(!s.authenticated(g, a, true, true));
    g = s.begin(oldAddress);
    s.observe(g, a);
    s.confirm(g, "Device", true);
    s.observe(g, b);
    QVERIFY(!s.authenticated(g, b, true, true));
  }
  void persistedIdentityRoundTripAndAtomicFailure()
  {
    IdentityBindings s;
    seed(s);
    const auto saved = s.encode();
    IdentityBindings copy;
    QVERIFY(copy.load(saved));
    QCOMPARE(copy.devices(), s.devices());
    auto doc = QJsonDocument::fromJson(saved).object();
    auto array = doc["devices"].toArray();
    array.append(array[0]);
    doc["devices"] = array;
    QVERIFY(!copy.load(QJsonDocument(doc).toJson()));
    QCOMPARE(copy.encode(), saved);
    QVERIFY(!copy.load(QByteArray(32769, 'x')));
    QCOMPARE(copy.encode(), saved);
    QTemporaryDir d;
    QFile f(d.path() + "/synthetic-device-metadata.json");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(saved);
    f.close();
    QVERIFY(f.open(QIODevice::ReadOnly));
    IdentityBindings disk;
    QVERIFY(disk.load(f.readAll()));
    QCOMPARE(disk.encode(), saved);
  }
  void rejectMalformedIdentity_data()
  {
    QTest::addColumn<QString>("fp");
    QTest::newRow("short") << QString(40, 'a');
    QTest::newRow("nonhex") << QString(64, 'g');
    QTest::newRow("whitespace") << (' ' + QString(64, 'a'));
    QTest::newRow("newline") << (QString(64, 'a') + '\n');
    QTest::newRow("separator-confusion") << (QString(63, 'a') + ':');
  }
  void rejectMalformedIdentity()
  {
    QFETCH(QString, fp);
    QVERIFY(IdentityBindings::normalizedFingerprint(fp).isEmpty());
    IdentityBindings s;
    QCOMPARE(s.begin(oldAddress, fp), quint64(0));
  }
  void boundedDeviceStore()
  {
    IdentityBindings s;
    for (int i = 0; i < 16; ++i) {
      const auto fp = QString::number(i, 16).rightJustified(64, '0');
      const auto g = s.begin(oldAddress);
      s.observe(g, fp);
      QVERIFY(s.confirm(g, "Device", true));
      QVERIFY(s.authenticated(g, fp, true, true));
    }
    const auto g = s.begin(newAddress);
    s.observe(g, a);
    QVERIFY(!s.confirm(g, "Overflow", true));
    QCOMPARE(s.devices().size(), 16);
  }
  void pairingRequiresDeliberateComparisonAndSupportsChinese()
  {
    IdentityBindings s;
    auto g = s.begin(oldAddress);
    s.observe(g, a);
    PairingReviewDialog dialog(s, c);
    dialog.show();
    auto *button = dialog.findChild<QPushButton *>("pairingConfirm");
    QVERIFY(!button->isEnabled());
    dialog.accept();
    QVERIFY(s.devices().isEmpty());
    dialog.findChild<QLineEdit *>("pairingDisplayName")->setText(QString::fromUtf8("研发 <QA> & 设计"));
    dialog.findChild<QCheckBox *>("pairingCompared")->setChecked(true);
    QVERIFY(button->isEnabled());
    dialog.accept();
    QCOMPARE(dialog.result(), int(QDialog::Accepted));
    QVERIFY(s.devices().isEmpty());
    QVERIFY(s.authenticated(g, a, true, true));
  }
  void changedIdentityCannotBeConfirmedAndCancelInvalidates()
  {
    IdentityBindings s;
    seed(s);
    const auto before = s.encode();
    const auto g = s.begin(newAddress, a);
    s.observe(g, b);
    PairingReviewDialog dialog(s, c);
    QVERIFY(!dialog.findChild<QCheckBox *>("pairingCompared")->isEnabled());
    dialog.accept();
    QVERIFY(dialog.result() != QDialog::Accepted);
    QCOMPARE(s.encode(), before);
    dialog.reject();
    QVERIFY(!s.authenticated(g, a, true, true));
  }
  void pairingKeyboardCancelInvalidatesPendingIdentity_data()
  {
    QTest::addColumn<int>("key");
    QTest::newRow("return-defaults-to-cancel") << int(Qt::Key_Return);
    QTest::newRow("escape-cancels") << int(Qt::Key_Escape);
  }
  void pairingKeyboardCancelInvalidatesPendingIdentity()
  {
    QFETCH(int, key);
    IdentityBindings s;
    const auto g = s.begin(oldAddress);
    s.observe(g, a);
    PairingReviewDialog dialog(s, c);
    dialog.show();
    auto *name = dialog.findChild<QLineEdit *>("pairingDisplayName");
    name->setText("Reviewed device");
    dialog.findChild<QCheckBox *>("pairingCompared")->setChecked(true);
    QVERIFY(dialog.findChild<QPushButton *>("pairingConfirm")->isEnabled());
    name->setFocus();
    QTest::keyClick(name, Qt::Key(key));
    QCOMPARE(dialog.result(), int(QDialog::Rejected));
    QVERIFY(!dialog.isVisible());
    QVERIFY(!s.authenticated(g, a, true, true));
    QVERIFY(s.devices().isEmpty());
  }
  void expiredAndInvalidNameReviewCannotPersist()
  {
    IdentityBindings s;
    const auto g = s.begin(oldAddress);
    s.observe(g, a);
    PairingReviewDialog dialog(s, c);
    auto *name = dialog.findChild<QLineEdit *>("pairingDisplayName");
    auto *check = dialog.findChild<QCheckBox *>("pairingCompared");
    check->setChecked(true);
    name->setText(QString(81, 'W'));
    QVERIFY(!dialog.findChild<QPushButton *>("pairingConfirm")->isEnabled());
    QVERIFY(!dialog.findChild<QLabel *>("pairingError")->text().isEmpty());
    name->setText("Device");
    QVERIFY(dialog.findChild<QPushButton *>("pairingConfirm")->isEnabled());
    dialog.expireReview();
    dialog.accept();
    QVERIFY(dialog.result() != QDialog::Accepted);
    QVERIFY(!s.authenticated(g, a, true, true));
    QVERIFY(s.devices().isEmpty());
  }
  void diagnosticsExportDropsEveryUnrecognizedValue()
  {
    ConnectionDiagnostics d;
    d.beginAttempt(false, "password-token-private-host.local", 34800, true, true);
    d.requireAttention(ConnectionDiagnostics::Attention::Protocol);
    auto o = QJsonDocument::fromJson(d.diagnosticSnapshot()).object();
    o["password"] = "secret-password";
    o["token"] = "secret-token";
    o["privateKey"] = "BEGIN PRIVATE KEY";
    o["clipboard"] = "private-clipboard";
    o["keystrokes"] = "private-input";
    o["path"] = "/Users/private-user";
    auto events = o["events"].toArray();
    events.append(QJsonObject{{"event", "secret-event"}, {"atUtc", "private-time"}});
    auto event = events[0].toObject();
    event["password"] = "nested-secret";
    event["atUtc"] = "private-time";
    events[0] = event;
    o["events"] = events;
    const auto bytes = DiagnosticExportDialog::sanitized(QJsonDocument(o).toJson());
    QVERIFY(!bytes.isEmpty());
    for (const auto *secret :
         {"secret", "private", "password", "token", "clipboard", "keystrokes", "PRIVATE KEY", "atUtc", "192.168"})
      QVERIFY2(!bytes.contains(secret), secret);
    QCOMPARE(DiagnosticExportDialog::sanitized(bytes), bytes);
    const auto result = QJsonDocument::fromJson(bytes).object();
    QCOMPARE(result["inputDeliveryVerified"], false);
    QCOMPARE(result["redacted"], true);
    QTemporaryDir temp;
    const auto path = temp.path() + "/diagnostics.json";
    QVERIFY(DiagnosticExportDialog::saveNewFile(path, bytes));
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), bytes);
    file.close();
    QVERIFY(!DiagnosticExportDialog::saveNewFile(path, bytes));
    QVERIFY(!DiagnosticExportDialog::saveNewFile(temp.path() + "/bad.json", "{}"));
    QVERIFY(!QFileInfo::exists(temp.path() + "/bad.json"));
  }
  void diagnosticsLimitAndInvalidInput()
  {
    ConnectionDiagnostics d;
    for (int i = 0; i < 100; ++i)
      d.setDiscoveryActive(i % 2);
    const auto bytes = DiagnosticExportDialog::sanitized(d.diagnosticSnapshot());
    QVERIFY(!bytes.isEmpty());
    QCOMPARE(QJsonDocument::fromJson(bytes)["events"].toArray().size(), 64);
    QVERIFY(DiagnosticExportDialog::sanitized(QByteArray(32769, 'x')).isEmpty());
    auto o = QJsonDocument::fromJson(d.diagnosticSnapshot()).object();
    o["phase"] = "secret-phase";
    QVERIFY(DiagnosticExportDialog::sanitized(QJsonDocument(o).toJson()).isEmpty());
  }
  void monitorNegativeCoordinatesPortraitAndMirrors()
  {
    const auto t = MonitorTopology::inspect({{{-1080, 0, 1080, 1920}, 1}, {{0, 0, 1920, 1920}, 1}});
    QVERIFY(t.error.isEmpty());
    QVERIFY(t.rectangularAggregate());
    QCOMPARE(t.bounds, QRect(-1080, 0, 3000, 1920));
    const auto mirror = MonitorTopology::inspect({{{0, 0, 1920, 1080}, 1}, {{0, 0, 1920, 1080}, 1}});
    QVERIFY(mirror.mirrored);
    QCOMPARE(mirror.monitors.size(), 1);
    QVERIFY(mirror.rectangularAggregate());
  }
  void monitorGapsDpiAndOverlapAreExplicitlyUnsupported()
  {
    const auto gap = MonitorTopology::inspect({{{0, 0, 1920, 1080}, 1}, {{1920, 200, 2560, 1440}, 2}});
    QVERIFY(gap.hasGaps);
    QVERIFY(gap.mixedScale);
    QVERIFY(!gap.rectangularAggregate());
    const auto overlap = MonitorTopology::inspect({{{0, 0, 1920, 1080}, 1}, {{100, 0, 1920, 1080}, 1}});
    QVERIFY(overlap.hasOverlap);
    QVERIFY(!overlap.rectangularAggregate());
    QVERIFY(!MonitorTopology::inspect({{{0, 0, 1, 1}, std::numeric_limits<double>::quiet_NaN()}}).error.isEmpty());
    const auto huge = MonitorTopology::inspect({{{0, 0, 40000, 1000}, 1}});
    QVERIFY(huge.legacyRangeExceeded);
    QVERIFY(!huge.rectangularAggregate());
    DisplayTopologyDialog dialog(gap);
    QCOMPARE(dialog.findChild<QTableWidget *>("monitorGeometryTable")->rowCount(), 2);
    QVERIFY(dialog.findChild<QLabel *>("monitorProtocolLimit")->text().contains("unavailable"));
  }
  void newDialogGeometry_data()
  {
    QTest::addColumn<int>("kind");
    QTest::newRow("pairing") << 0;
    QTest::newRow("identity-change") << 1;
    QTest::newRow("diagnostics") << 2;
    QTest::newRow("displays") << 3;
    QTest::newRow("saved-devices") << 4;
    QTest::newRow("saved-devices-empty") << 5;
  }
  void newDialogGeometry()
  {
    QFETCH(int, kind);
    IdentityBindings bindings;
    if (kind == 1)
      seed(bindings);
    auto g = bindings.begin(newAddress, kind == 1 ? a : QString());
    bindings.observe(g, b);
    ConnectionDiagnostics diagnostic;
    QDialog *dialog = nullptr;
    if (kind < 2)
      dialog = new PairingReviewDialog(bindings, c);
    if (kind == 2)
      dialog = new DiagnosticExportDialog(diagnostic.diagnosticSnapshot());
    if (kind == 3)
      dialog =
          new DisplayTopologyDialog(MonitorTopology::inspect({{{-1920, 0, 1920, 1080}, 1}, {{0, 0, 2560, 1440}, 2}}));
    if (kind >= 4) {
      if (kind == 4) {
        seed(bindings);
        QVERIFY(bindings.rename(a, QString::fromUtf8("研发 <QA> & 设计 ") + QString(60, 'W')));
      }
      dialog = new TrustedDevicesDialog(bindings, true);
      if (kind == 4)
        dialog->findChild<QListWidget *>("trustedDeviceList")->setCurrentRow(0);
    }
    dialog->resize(dialog->minimumSize());
    dialog->show();
    QTest::qWait(20);
    if (kind < 2)
      QVERIFY(dialog->width() >= 540);
    for (auto *button : dialog->findChildren<QPushButton *>())
      if (button->isVisible()) {
        const auto rect = QRect(button->mapTo(dialog, QPoint(0, 0)), button->size());
        QVERIFY2(dialog->rect().contains(rect), qPrintable(button->text()));
        QVERIFY2(
            button->width() >= button->fontMetrics().horizontalAdvance(button->text()) + 12, qPrintable(button->text())
        );
      }
    for (auto *label : dialog->findChildren<QLabel *>())
      if (label->isVisible() && label->wordWrap()) {
        const auto needed = label->fontMetrics()
                                .boundingRect(QRect(0, 0, label->width(), 10000), Qt::TextWordWrap, label->text())
                                .height();
        QVERIFY2(label->height() + 3 >= needed, qPrintable(label->text()));
      }
    delete dialog;
  }
  void actualSettingsRejectNamespaceAndSecurityChanges()
  {
    QTemporaryDir temp;
    temp.setAutoRemove(false);
    const auto parent = QFileInfo(temp.path()).canonicalFilePath();
    const auto path = fixture(parent);
    const auto contract = IsolatedSession::inspect(path, parent);
    QVERIFY(contract.valid());
    QVERIFY(IsolatedSession::activate(path, parent));
    QCOMPARE(IsolatedSession::active()->ipcName, contract.ipcName);
    const auto outside = parent + "/production-sentinel.conf";
    QFile sentinel(outside);
    QVERIFY(sentinel.open(QIODevice::WriteOnly));
    sentinel.write("do-not-import-or-change");
    sentinel.close();
    qputenv("XDG_CONFIG_HOME", parent.toUtf8());
    qputenv("XDG_STATE_HOME", parent.toUtf8());
    QCOMPARE(Settings::settingsFile(), contract.settings);
    QCOMPARE(Settings::settingsPath(), QFileInfo(contract.settings).absolutePath());
    QCOMPARE(Settings::tlsTrustedServersDb(), contract.trustedServers);
    QCOMPARE(Settings::tlsTrustedClientsDb(), contract.trustedClients);
    QCOMPARE(Settings::value(Settings::Core::Port).toInt(), 34800);
    QCOMPARE(Settings::value(Settings::Security::Certificate).toString(), contract.identity);
    Settings::setSettingsFile(outside);
    Settings::setStateFile(outside);
    Settings::setValue(Settings::Core::Port, 24800);
    Settings::setValue(Settings::Security::Certificate, outside);
    Settings::setValue(Settings::Security::TlsEnabled, false);
    Settings::setValue(Settings::Security::CheckPeers, false);
    Settings::setValue(Settings::Core::ProcessMode, Settings::Service);
    Settings::setValue(Settings::Client::LanguageSync, true);
    Settings::setValue(Settings::Server::EnableClipboard, true);
    QCOMPARE(Settings::settingsFile(), contract.settings);
    QCOMPARE(Settings::value(Settings::Core::Port).toInt(), 34800);
    QVERIFY(Settings::value(Settings::Security::TlsEnabled).toBool());
    QVERIFY(Settings::value(Settings::Security::CheckPeers).toBool());
    QCOMPARE(Settings::value(Settings::Security::Certificate).toString(), contract.identity);
    QCOMPARE(Settings::value(Settings::Core::ProcessMode).toInt(), int(Settings::Desktop));
    QVERIFY(!Settings::value(Settings::Client::LanguageSync).toBool());
    QVERIFY(!Settings::value(Settings::Server::EnableClipboard).toBool());
    Settings::setValue(Settings::Gui::WindowGeometry, QByteArray("synthetic-geometry"));
    Settings::save(false);
    QVERIFY(QFileInfo::exists(contract.state));
    QSettings state(contract.state, QSettings::IniFormat);
    QCOMPARE(state.value(Settings::Gui::WindowGeometry).toByteArray(), QByteArray("synthetic-geometry"));
    QVERIFY(!QFileInfo::exists(contract.identity));
    QVERIFY(!QFileInfo::exists(contract.trustedServers));
    QVERIFY(!QFileInfo::exists(contract.trustedClients));
    QVERIFY(!IsolatedSession::activate(path, parent));
    QVERIFY(sentinel.open(QIODevice::ReadOnly));
    QCOMPARE(sentinel.readAll(), QByteArray("do-not-import-or-change"));
  }
};
int main(int argc, char **argv)
{
  QApplication app(argc, argv);
  app.setStyle("Fusion");
  QTranslator translator;
  const auto translation = qEnvironmentVariable("DESKFLOW_TEST_TRANSLATION");
  if (!translation.isEmpty()) {
    if (!translator.load(translation))
      return 5;
    app.installTranslator(&translator);
  }
  if (qEnvironmentVariable("DESKFLOW_TEST_THEME") == "dark") {
    QPalette p;
    p.setColor(QPalette::Window, QColor("#202630"));
    p.setColor(QPalette::WindowText, QColor("#e7edf5"));
    p.setColor(QPalette::Base, QColor("#18202c"));
    p.setColor(QPalette::Text, QColor("#e7edf5"));
    p.setColor(QPalette::Button, QColor("#303b4b"));
    p.setColor(QPalette::ButtonText, QColor("#e7edf5"));
    app.setPalette(p);
  }
  Stage10OfflineTests tests;
  return QTest::qExec(&tests, argc, argv);
}
#include "Stage10OfflineTests.moc"
