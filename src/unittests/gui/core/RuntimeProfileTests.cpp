// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "common/IsolatedSession.h"
#include "common/RuntimeProfile.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>
using namespace deskflow::lab;
class RuntimeProfileTests : public QObject
{
  Q_OBJECT
  RuntimeProfile candidate(int port = 24800) const
  {
    return {RuntimeProfile::Mode::Candidate, port, true, true, true};
  }
  QByteArray snapshot() const
  {
    return QJsonDocument(
               QJsonObject{
                   {"schema", 1},
                   {"computerName", "办公室 Mac"},
                   {"remoteHost", "192.168.50.20"},
                   {"port", 34800},
                   {"tls", true},
                   {"peerChecks", true},
                   {"languageSync", false},
                   {"identityPin", QString(64, 'a')},
                   {"autoStart", true},
                   {"processMode", "desktop"}
               }
    )
        .toJson();
  }
private Q_SLOTS:
  void validProfilesRemainHardDisabled_data()
  {
    QTest::addColumn<int>("mode");
    QTest::addColumn<int>("port");
    QTest::newRow("preview") << 0 << 34800;
    QTest::newRow("test") << 1 << 34800;
    QTest::newRow("standard") << 2 << 24800;
    QTest::newRow("custom") << 2 << 45000;
  }
  void validProfilesRemainHardDisabled()
  {
    QFETCH(int, mode);
    QFETCH(int, port);
    RuntimeProfile p{static_cast<RuntimeProfile::Mode>(mode), port, true, true, mode == 2};
    QVERIFY(p.valid());
    QVERIFY(!p.executable());
    bool ok = false;
    QCOMPARE(RuntimeProfile::decode(p.encode(), &ok).encode(), p.encode());
    QVERIFY(ok);
    QCOMPARE(p.preflight()["canExecute"], false);
    QCOMPARE(p.preflight()["inputEnabled"], false);
    QCOMPARE(p.preflight()["writesPerformed"], false);
    QCOMPARE(p.permitsPort(port), mode != 0);
    QVERIFY(!p.permitsPort(port + 1));
  }
  void badProfile_data()
  {
    QTest::addColumn<QString>("kind");
    for (const auto *s :
         {"tls", "peerChecks", "input", "port", "fraction", "mode", "schema", "extra", "size", "garbage"})
      QTest::newRow(s) << QString(s);
  }
  void badProfile()
  {
    QFETCH(QString, kind);
    auto o = QJsonDocument::fromJson(candidate().encode()).object();
    if (kind == "tls" || kind == "peerChecks" || kind == "input")
      o[kind] = false;
    if (kind == "port")
      o["port"] = 22;
    if (kind == "fraction")
      o["port"] = 24800.5;
    if (kind == "mode")
      o["mode"] = "production";
    if (kind == "schema")
      o["schema"] = 2;
    if (kind == "extra")
      o["permissionOverride"] = true;
    auto bytes = QJsonDocument(o).toJson(QJsonDocument::Compact);
    if (kind == "size")
      bytes = QByteArray(1025, 'x');
    if (kind == "garbage")
      bytes += "x";
    bool ok = true;
    const auto p = RuntimeProfile::decode(bytes, &ok);
    QVERIFY(!ok);
    QVERIFY(!p.valid());
    QVERIFY(!p.executable());
  }
  void publicMigrationPreservesPinAndLanguageDisablesStartupAndRollsBackExactly()
  {
    const auto original = snapshot();
    const auto m = ProfileMigration::prepare(original, candidate());
    QVERIFY2(m.valid(), qPrintable(m.error));
    const auto after = QJsonDocument::fromJson(m.after).object();
    QCOMPARE(after["identityPin"], QString(64, 'a'));
    QCOMPARE(after["languageSync"], false);
    QCOMPARE(after["autoStart"], false);
    QCOMPARE(after["port"], 24800);
    QCOMPARE(m.rollback(m.after), original);
    QString error;
    QVERIFY(m.rollback(m.after + " ", &error).isEmpty());
    QVERIFY(!error.isEmpty());
    const auto plan = QJsonDocument(m.plan()).toJson();
    QVERIFY(!plan.contains("192.168.50.20"));
    QVERIFY(!plan.contains(QByteArray(64, 'a')));
    QCOMPARE(m.plan()["writesPerformed"], false);
  }
  void unsafeMigration_data()
  {
    QTest::addColumn<QString>("kind");
    for (const auto *s :
         {"privateKey", "password", "token", "trustFile", "tls", "peerChecks", "languageSync", "identityPin",
          "processMode", "remoteHost", "computerName", "schema"})
      QTest::newRow(s) << QString(s);
  }
  void unsafeMigration()
  {
    QFETCH(QString, kind);
    auto o = QJsonDocument::fromJson(snapshot()).object();
    if (kind == "privateKey" || kind == "password" || kind == "token" || kind == "trustFile")
      o[kind] = "SYNTHETIC_SECRET_SENTINEL";
    else if (kind == "tls" || kind == "peerChecks")
      o[kind] = false;
    else if (kind == "languageSync")
      o[kind] = true;
    else if (kind == "identityPin")
      o[kind] = "";
    else if (kind == "processMode")
      o[kind] = "service";
    else if (kind == "schema")
      o[kind] = 2;
    else
      o[kind] = "bad\nvalue";
    const auto m = ProfileMigration::prepare(QJsonDocument(o).toJson(), candidate());
    QVERIFY(!m.valid());
    QVERIFY(m.before.isEmpty());
    QVERIFY(m.after.isEmpty());
    QVERIFY(!QJsonDocument(m.plan()).toJson().contains("SENTINEL"));
  }
  void newCandidateSessionRetainsSeparatePathsAndRejectsRedirection()
  {
    QTemporaryDir temp(QFileInfo(QDir::tempPath()).canonicalFilePath() + "/runtime-profile-XXXXXX");
    QVERIFY(temp.isValid());
    const auto id = QUuid::createUuid().toString(QUuid::Id128), root = temp.path() + '/' + id;
    for (const auto *d : {"config/DeskflowLab/tls", "state", "runtime", "logs"})
      QVERIFY(QDir().mkpath(root + '/' + d));
    QJsonObject manifest{
        {"schema", 2},
        {"purpose", "deskflow-lab-network"},
        {"session", id},
        {"mode", "client"},
        {"bindAddress", "192.168.50.10"},
        {"peerAddress", "192.168.50.20"},
        {"port", 24800},
        {"tls", true},
        {"peerChecks", true},
        {"runtime", QString::fromUtf8(candidate().encode())}
    };
    auto save = [&] {
      QFile f(root + "/session.json");
      if (!f.open(QIODevice::WriteOnly))
        return false;
      return f.write(QJsonDocument(manifest).toJson()) > 0;
    };
    QVERIFY(save());
    auto s = IsolatedSession::inspect(root, temp.path());
    QVERIFY2(s.valid(), qPrintable(s.error));
    QCOMPARE(s.port, 24800);
    QVERIFY(s.settings.startsWith(root + '/'));
    QVERIFY(s.identity.startsWith(root + '/'));
    QVERIFY(s.ipcName.endsWith(id));
    QVERIFY(!s.runtime.executable());
    manifest["port"] = 34800;
    QVERIFY(save());
    QVERIFY(!IsolatedSession::inspect(root, temp.path()).valid());
    manifest["port"] = 24800;
    manifest["identityPath"] = "/production/secret";
    QVERIFY(save());
    QVERIFY(!IsolatedSession::inspect(root, temp.path()).valid());
    QVERIFY(!IsolatedSession::inspect("/Applications/Deskflow.app", temp.path()).valid());
  }
};
QTEST_GUILESS_MAIN(RuntimeProfileTests)
#include "RuntimeProfileTests.moc"
