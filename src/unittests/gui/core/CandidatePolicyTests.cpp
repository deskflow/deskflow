// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "common/CandidatePolicy.h"
#include "common/CredentialStore.h"
#include "common/ServiceSession.h"
#include "common/Settings.h"
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QXmlStreamReader>
using namespace deskflow::candidate;
class CandidatePolicyTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void clearedProbeMetadataDoesNotExhaustRecoveryLimit()
  {
    QTemporaryDir dir;
    const auto root = QFileInfo(dir.path()).canonicalFilePath() + "/DeskflowCandidate";
    QVERIFY(QDir().mkpath(root));
    const auto settings = root + "/DeskflowCandidate.conf", parent = root + "/recovery-sessions";
    QVERIFY(deskflow::lab::CredentialStore::ensurePrivateCandidateDirectory(parent));
    for (int i = 0; i < 130; ++i)
      QVERIFY(QDir().mkdir(parent + '/' + QString::number(i, 16).rightJustified(32, '0')));
    QVERIFY(deskflow::lab::CredentialStore::recoverCandidateSessions(settings));
    const auto bad = parent + '/' + QString(32, 'a');
    QVERIFY(QFile::link(root, bad));
    QVERIFY(!deskflow::lab::CredentialStore::recoverCandidateSessions(settings));
  }
  void candidateProfileReacquiredAfterDestructionAndRevocationRefused()
  {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto root = QFileInfo(dir.path()).canonicalFilePath() + "/DeskflowCandidate";
    QVERIFY(QDir().mkpath(root));
    const auto settings = root + "/DeskflowCandidate.conf", identity = root + "/tls/deskflow-candidate.pem";
    QVERIFY(deskflow::lab::CredentialStore::ensurePrivateCandidateDirectory(root + "/tls"));
    QFile file(identity);
    QVERIFY(deskflow::lab::CredentialStore::openPrivateCandidateFile(file));
    file.write("SYNTHETIC-NOT-A-KEY");
    file.close();
    const auto fp = QString(64, 'a');
    QVERIFY(
        deskflow::lab::CredentialStore::writeCandidateTrust(
            root + "/tls/trusted-servers", ("v2:sha256:" + fp + "\n").toUtf8()
        )
    );
    QString error;
    auto lease = deskflow::lab::CredentialStore::acquireCandidateProfile(settings, identity, fp, &error);
    QVERIFY2(lease, qPrintable(error));
    QCOMPARE(lease->material().identity, QByteArray("SYNTHETIC-NOT-A-KEY"));
    lease.reset();
    auto second = deskflow::lab::CredentialStore::acquireCandidateProfile(settings, identity, fp, &error);
    QVERIFY(second);
    second.reset();
    QVERIFY(
        deskflow::lab::CredentialStore::writeCandidateTrust(
            root + "/tls/trusted-servers", ("v2:sha256:" + QString(64, 'b') + "\n").toUtf8()
        )
    );
    QVERIFY(!deskflow::lab::CredentialStore::acquireCandidateProfile(settings, identity, fp, &error));
    QCOMPARE(error, QString("candidate-identity-not-trusted"));
    QVERIFY(!deskflow::lab::CredentialStore::acquireCandidateProfile(settings, root + "/Deskflow/tls/key", fp, &error));
    QVERIFY(QFile::remove(identity));
    QVERIFY(!deskflow::lab::CredentialStore::acquireCandidateProfile(settings, identity, QString(64, 'b'), &error));
  }
  void profileRejectsBroadModeAndSymbolicLinks()
  {
    QTemporaryDir dir;
    const auto root = QFileInfo(dir.path()).canonicalFilePath() + "/DeskflowCandidate";
    QVERIFY(QDir().mkpath(root));
    QVERIFY(deskflow::lab::CredentialStore::ensurePrivateCandidateDirectory(root + "/tls"));
    const auto settings = root + "/DeskflowCandidate.conf", identity = root + "/tls/deskflow-candidate.pem",
               trust = root + "/tls/trusted-servers", fp = QString(64, 'a');
    QFile f(identity);
    QVERIFY(deskflow::lab::CredentialStore::openPrivateCandidateFile(f));
    f.write("SYNTHETIC-NOT-A-KEY");
    f.close();
    QVERIFY(deskflow::lab::CredentialStore::writeCandidateTrust(trust, ("v2:sha256:" + fp + "\n").toUtf8()));
    QVERIFY(f.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ReadOther));
    QString error;
    QVERIFY(!deskflow::lab::CredentialStore::acquireCandidateProfile(settings, identity, fp, &error));
    QVERIFY(f.setPermissions(QFile::ReadOwner | QFile::WriteOwner));
    QVERIFY(QFile::remove(trust));
    QVERIFY(QFile::link(identity, trust));
    QVERIFY(!deskflow::lab::CredentialStore::acquireCandidateProfile(settings, identity, fp, &error));
  }
  void serviceSessionBindsContextAndRejectsUnsafePath()
  {
    deskflow::candidate::ServiceSession s{
        "/private/tmp/profile/DeskflowCandidate/DeskflowCandidate.conf", {QString(32, 'a'), QString(64, 'b'), 9}
    };
    QVERIFY(s.valid());
    auto copy = deskflow::candidate::ServiceSession::decode(s.encode());
    QCOMPARE(copy.context, s.context);
    QCOMPARE(copy.settings, s.settings);
    QVERIFY(copy.arguments().contains("--peer-auth-context " + s.context.encode()));
    s.settings = "/private/tmp/Deskflow/Deskflow.conf";
    QVERIFY(!s.valid());
    s.settings = "/private/tmp/bad\"path/DeskflowCandidate/DeskflowCandidate.conf";
    QVERIFY(!s.valid());
    QVERIFY(!deskflow::candidate::ServiceSession::decode("garbage").valid());
  }

  void pauseDominatesAutoStart_data()
  {
    QTest::addColumn<bool>("autoStart");
    QTest::addColumn<bool>("paused");
    QTest::addColumn<bool>("expected");
    QTest::newRow("off") << false << false << false;
    QTest::newRow("paused-off") << false << true << false;
    QTest::newRow("resume") << true << false << true;
    QTest::newRow("paused-on") << true << true << false;
  }
  void pauseDominatesAutoStart()
  {
    QFETCH(bool, autoStart);
    QFETCH(bool, paused);
    QFETCH(bool, expected);
    QCOMPARE(mayResume(autoStart, paused), expected);
  }
  void paths_data()
  {
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("ok");
    QTest::newRow("own") << QString("/private/tmp/new-candidate/DeskflowCandidate/DeskflowCandidate.conf") << true;
    for (const auto &v : QStringList{
             "/private/tmp/Deskflow/Deskflow.conf", "/private/tmp/DeskflowCandidate/Deskflow.conf",
             "/private/tmp/Deskflow/DeskflowCandidate.conf", "relative/DeskflowCandidate/DeskflowCandidate.conf",
             "/private/tmp/DeskflowCandidate/../DeskflowCandidate/DeskflowCandidate.conf",
             "//remote/DeskflowCandidate/DeskflowCandidate.conf"
         })
      QTest::newRow(qPrintable(v)) << v << false;
  }
  void paths()
  {
    QFETCH(QString, path);
    QFETCH(bool, ok);
    QCOMPARE(settingsPath(path), ok);
  }
  void refusesLinkedConfigAndCredentials()
  {
    QTemporaryDir d;
    QVERIFY(d.isValid());
    const auto root = QFileInfo(d.path()).canonicalFilePath() + "/DeskflowCandidate";
    QVERIFY(QDir().mkpath(root + "/tls"));
    QFile target(root + "/unrelated");
    QVERIFY(target.open(QIODevice::WriteOnly));
    target.write("not a credential");
    target.close();
    const auto settings = root + "/DeskflowCandidate.conf", cert = root + "/tls/deskflow-candidate.pem";
    QVERIFY(securityProfile(settings, cert, true, true));
    QVERIFY(!securityProfile(settings, cert, false, true));
    QVERIFY(!securityProfile(settings, cert, true, false));
    QVERIFY(!securityProfile(settings, "/private/tmp/Deskflow/tls/deskflow.pem", true, true));
    QVERIFY(QFile::link(target.fileName(), settings));
    QVERIFY(!settingsPath(settings));
    QVERIFY(QFile::remove(settings));
    QVERIFY(QFile::link(target.fileName(), root + "/tls/trusted-servers"));
    QVERIFY(!securityProfile(settings, cert, true, true));
  }
  void preferencesSurviveFreshSettingsProcessAndUnknownKeysAreRemoved()
  {
    const auto parent = qEnvironmentVariable("DESKFLOW_TEST_ROOT");
    QVERIFY(!parent.isEmpty());
    QTemporaryDir dir(parent + "/candidate-settings-XXXXXX");
    QVERIFY(dir.isValid());
    const auto root = QFileInfo(dir.path()).canonicalFilePath();
    QVERIFY(QDir().mkpath(root + "/config/DeskflowLab"));
    QVERIFY(QDir().mkpath(root + "/state"));
    QSettings settings(root + "/config/DeskflowLab/DeskflowLab.conf", QSettings::IniFormat);
    settings.setValue("gui/userPaused", true);
    settings.setValue("gui/identityBindings", "synthetic-bindings");
    settings.setValue("gui/automaticRecovery", "synthetic-recovery");
    settings.setValue("gui/runtimeProfile", "synthetic-runtime");
    settings.setValue("gui/unknown-obsolete-key", "remove me");
    settings.sync();
    QCOMPARE(settings.status(), QSettings::NoError);
    QProcess child;
    child.start(QCoreApplication::applicationFilePath(), {"--settings-fixture", root});
    QVERIFY(child.waitForFinished(5000));
    QCOMPARE(child.exitStatus(), QProcess::NormalExit);
    QCOMPARE(child.exitCode(), 0);
    settings.sync();
    QVERIFY(!settings.contains("gui/unknown-obsolete-key"));
    QVERIFY(settings.value("gui/userPaused").toBool());
  }
  void privateCandidateFileIsExclusiveAndContainsOnlySyntheticFixture()
  {
    QTemporaryDir d;
    QVERIFY(d.isValid());
    const auto root = QFileInfo(d.path()).canonicalFilePath() + "/DeskflowCandidate";
    QVERIFY(QDir().mkpath(root + "/tls"));
    const auto path = root + "/tls/deskflow-candidate.pem";
    QFile f(path);
    QVERIFY(deskflow::lab::CredentialStore::openPrivateCandidateFile(f));
    const auto permissions = f.permissions();
    QVERIFY(!(permissions & (QFile::ReadGroup | QFile::WriteGroup | QFile::ReadOther | QFile::WriteOther)));
    QCOMPARE(f.write("SYNTHETIC-NOT-A-KEY"), qint64(19));
    f.close();
    QFile duplicate(path);
    QVERIFY(!deskflow::lab::CredentialStore::openPrivateCandidateFile(duplicate));
    QVERIFY(f.open(QIODevice::ReadOnly));
    QCOMPARE(f.readAll(), QByteArray("SYNTHETIC-NOT-A-KEY"));
    f.close();
    QFile foreign(root + "/tls/deskflow.pem");
    QVERIFY(!deskflow::lab::CredentialStore::openPrivateCandidateFile(foreign));
    QVERIFY(!foreign.exists());
  }
  void nativeLaunchPlanEscapesPathAndRestartsCrashesOnly()
  {
    const auto bytes =
        launchAgent("/private/tmp/candidate & 中文/DeskflowCandidate.app/Contents/MacOS/DeskflowCandidate");
    QVERIFY(!bytes.isEmpty());
    QXmlStreamReader reader(bytes);
    while (!reader.atEnd())
      reader.readNext();
    QVERIFY(!reader.hasError());
    QVERIFY(bytes.contains("&amp;"));
    QVERIFY(bytes.contains("SuccessfulExit"));
    QVERIFY(bytes.contains("<false/>"));
    QVERIFY(bytes.contains("ThrottleInterval"));
    QVERIFY(!bytes.contains("local.deskflow"));
    QVERIFY(launchAgent("/Applications/Deskflow.app/Contents/MacOS/Deskflow").isEmpty());
  }
};
int main(int argc, char **argv)
{
  if (argc == 3 && QByteArray(argv[1]) == "--settings-fixture") {
    const auto parent = QFileInfo(qEnvironmentVariable("DESKFLOW_TEST_ROOT")).canonicalFilePath();
    const auto root = QString::fromLocal8Bit(argv[2]);
    if (parent.isEmpty() || QFileInfo(root).canonicalFilePath() != root || QFileInfo(root).absolutePath() != parent ||
        !QFileInfo(root).fileName().startsWith("candidate-settings-"))
      return 2;
    qputenv("XDG_CONFIG_HOME", (root + "/config").toUtf8());
    qputenv("XDG_STATE_HOME", (root + "/state").toUtf8());
    QCoreApplication app(argc, argv);
    const bool ok = Settings::value("gui/userPaused").toBool() &&
                    Settings::value("gui/identityBindings").toString() == "synthetic-bindings" &&
                    Settings::value("gui/automaticRecovery").toString() == "synthetic-recovery" &&
                    Settings::value("gui/runtimeProfile").toString() == "synthetic-runtime" &&
                    !Settings::value("gui/unknown-obsolete-key").isValid();
    Settings::save(false);
    return ok ? 0 : 3;
  }
  QCoreApplication app(argc, argv);
  CandidatePolicyTests tests;
  return QTest::qExec(&tests, argc, argv);
}
#include "CandidatePolicyTests.moc"
