// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "gui/core/WindowsStartupProvider.h"
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>
#include <QTest>
using namespace deskflow::gui::winstartup;
class WindowsStartupProviderTests : public QObject
{
  Q_OBJECT
  struct Fake : System
  {
    State state;
    int writes = 0;
    Effect effect = Effect::Applied;
    bool race = false;
    Fake()
    {
      state.available = true;
      state.userSid = "S-1-5-21-1234";
      state.executable = qEnvironmentVariable("DESKFLOW_TEST_ROOT") + "/Candidate.exe";
      state.executableHash = QByteArray(64, 'a');
    }
    State read() override
    {
      return state;
    }
    Effect replace(const QByteArray &xml, const QByteArray &expected) override
    {
      if (race)
        state.executableHash = QByteArray(64, 'b');
      if (state.revision() != expected)
        return Effect::Unchanged;
      ++writes;
      if (effect == Effect::Applied) {
        state.definition = xml;
        state.exists = !xml.isEmpty();
      }
      return effect;
    }
  };
private Q_SLOTS:
  void changeAtWriteBoundaryNeverOverwrites()
  {
    Fake f;
    auto p = prepare(f.state, {true, true});
    f.race = true;
    QVERIFY(!apply(p, p.digest, f).success);
    QCOMPARE(f.writes, 0);
  }
  void independentProcessRestoresBackup()
  {
    QTemporaryDir dir(qEnvironmentVariable("DESKFLOW_TEST_ROOT") + "/startup-process-XXXXXX");
    QVERIFY(dir.isValid());
    Fake f;
    const auto p = prepare(f.state, {true, true});
    const auto path = dir.path() + "/backup.json";
    QVERIFY(applyRecorded(p, p.digest, f, path).success);
    QProcess child;
    child.start(QCoreApplication::applicationFilePath(), {"--restore-fixture", path});
    QVERIFY(child.waitForFinished(15000));
    QCOMPARE(child.exitStatus(), QProcess::NormalExit);
    QCOMPARE(child.exitCode(), 0);
  }
  void loginAndRecoveryUseOneLeastPrivilegeTask()
  {
    Fake f;
    const auto p = prepare(f.state, {true, true});
    QVERIFY(p.error.isEmpty());
    QVERIFY(p.definition.contains("InteractiveToken"));
    QVERIFY(p.definition.contains("LeastPrivilege"));
    QVERIFY(p.definition.contains("IgnoreNew"));
    QVERIFY(p.definition.contains("--managed-start"));
    QVERIFY(p.definition.contains("<Count>3</Count>"));
    QVERIFY(apply(p, p.digest, f).success);
    QCOMPARE(f.writes, 1);
  }
  void normalLoginHasNoCrashRestart()
  {
    Fake f;
    const auto p = prepare(f.state, {true, false});
    QVERIFY(!p.definition.contains("RestartOnFailure"));
  }
  void recoveryRequiresLogin()
  {
    Fake f;
    QVERIFY(!prepare(f.state, {false, true}).error.isEmpty());
  }
  void noConfirmationNeverWrites()
  {
    Fake f;
    auto p = prepare(f.state, {true, true});
    QVERIFY(!apply(p, {}, f).success);
    QCOMPARE(f.writes, 0);
  }
  void alteredPlanNeverWrites()
  {
    Fake f;
    auto p = prepare(f.state, {true, true});
    p.definition.replace("LeastPrivilege", "HighestAvailable");
    QVERIFY(!apply(p, p.digest, f).success);
    QCOMPARE(f.writes, 0);
  }
  void staleIdentityNeverWrites()
  {
    Fake f;
    auto p = prepare(f.state, {true, true});
    f.state.executableHash = QByteArray(64, 'b');
    QVERIFY(!apply(p, p.digest, f).success);
    QCOMPARE(f.writes, 0);
  }
  void legacyConflictBlocks()
  {
    Fake f;
    f.state.conflict = true;
    QVERIFY(!prepare(f.state, {true, true}).error.isEmpty());
    QCOMPARE(f.writes, 0);
  }
  void idempotentAndDisable()
  {
    Fake f;
    auto p = prepare(f.state, {true, true});
    QVERIFY(apply(p, p.digest, f).success);
    p = prepare(f.state, {true, true});
    QVERIFY(apply(p, p.digest, f).success);
    QCOMPARE(f.writes, 1);
    p = prepare(f.state, {});
    QVERIFY(apply(p, p.digest, f).success);
    QVERIFY(!f.state.exists);
    QCOMPARE(f.writes, 2);
  }
  void failedWriteDoesNotClaimSaved()
  {
    Fake f;
    auto p = prepare(f.state, {true, true});
    f.effect = Effect::Unchanged;
    const auto r = apply(p, p.digest, f);
    QVERIFY(!r.success);
    QVERIFY(!r.uncertain);
    QVERIFY(!f.state.exists);
  }
  void uncertainWriteNeverRetries()
  {
    Fake f;
    auto p = prepare(f.state, {true, true});
    f.effect = Effect::Uncertain;
    QVERIFY(apply(p, p.digest, f).uncertain);
    QCOMPARE(f.writes, 1);
  }
  void schedulerDefaultsDoNotInvalidateReadback()
  {
    Fake f;
    auto p = prepare(f.state, {true, true});
    QVERIFY(apply(p, p.digest, f).success);
    f.state.definition.replace(
        "<Settings>", "<Settings><Enabled>true</Enabled><Hidden>false</Hidden><Priority>7</Priority>"
    );
    p = prepare(f.state, {true, true});
    QVERIFY(apply(p, p.digest, f).success);
    QCOMPARE(f.writes, 1);
  }
  void persistentBackupRestoresAcrossProviderInstances()
  {
    QTemporaryDir dir(qEnvironmentVariable("DESKFLOW_TEST_ROOT") + "/startup-XXXXXX");
    QVERIFY(dir.isValid());
    QVERIFY(dir.isValid());
    Fake f;
    auto p = prepare(f.state, {true, true});
    const auto path = dir.path() + "/backup.json";
    QVERIFY(applyRecorded(p, p.digest, f, path).success);
    Fake reopened;
    reopened.state = f.state;
    auto restore = prepareRestore(reopened.state, path);
    QVERIFY(restore.error.isEmpty());
    QVERIFY(apply(restore, restore.digest, reopened).success);
    QVERIFY(!reopened.state.exists);
  }
  void backupFailureNeverChangesSystem()
  {
    QTemporaryDir dir(qEnvironmentVariable("DESKFLOW_TEST_ROOT") + "/startup-XXXXXX");
    QVERIFY(dir.isValid());
    Fake f;
    auto p = prepare(f.state, {true, true});
    QVERIFY(!applyRecorded(p, p.digest, f, dir.path()).success);
    QCOMPARE(f.writes, 0);
  }
  void changedTaskCannotUseOldBackup()
  {
    QTemporaryDir dir(qEnvironmentVariable("DESKFLOW_TEST_ROOT") + "/startup-XXXXXX");
    QVERIFY(dir.isValid());
    Fake f;
    auto p = prepare(f.state, {true, true});
    const auto path = dir.path() + "/backup.json";
    QVERIFY(applyRecorded(p, p.digest, f, path).success);
    f.state.definition.replace("IgnoreNew", "Parallel");
    QVERIFY(!prepareRestore(f.state, path).error.isEmpty());
  }
  void pauseDominatesResume()
  {
    QVERIFY(!mayLaunch(true, true));
    QVERIFY(!mayLaunch(true, false));
    QVERIFY(!mayLaunch(false, false));
    QVERIFY(mayLaunch(false, true));
  }

public:
  static int restoreChild(const QString &path)
  {
    const auto root = QDir(qEnvironmentVariable("DESKFLOW_TEST_ROOT")).canonicalPath();
    const auto file = QFileInfo(path).canonicalFilePath();
    if (root.isEmpty() || file.isEmpty() || !file.startsWith(root + "/"))
      return 2;
    Fake f;
    const auto enabled = prepare(f.state, {true, true});
    f.state.definition = enabled.definition;
    f.state.exists = true;
    const auto plan = prepareRestore(f.state, path);
    if (!plan.error.isEmpty())
      return 3;
    const auto result = apply(plan, plan.digest, f);
    return result.success && !f.state.exists ? 0 : 4;
  }
};
int main(int argc, char **argv)
{
  QCoreApplication app(argc, argv);
  if (argc == 3 && QByteArray(argv[1]) == "--restore-fixture")
    return WindowsStartupProviderTests::restoreChild(QString::fromLocal8Bit(argv[2]));
  WindowsStartupProviderTests tests;
  return QTest::qExec(&tests, argc, argv);
}
#include "WindowsStartupProviderTests.moc"
