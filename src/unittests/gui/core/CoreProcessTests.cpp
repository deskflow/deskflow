/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: MIT
 */

#include "CoreProcessTests.h"

#include "common/Settings.h"
#include "gui/config/ServerConfig.h"
#include "gui/core/CoreProcess.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QUuid>

using namespace deskflow::gui;

namespace {

constexpr auto kIgnoreTerminateEnvironment = "DESKFLOW_CORE_PROCESS_TEST_IGNORE_TERMINATE";
constexpr auto kIpcNameEnvironment = "DESKFLOW_CORE_PROCESS_TEST_IPC_NAME";
constexpr auto kGracefulStopResultEnvironment = "DESKFLOW_CORE_PROCESS_TEST_GRACEFUL_STOP_RESULT";

class ScopedEnvironmentVariable
{
public:
  explicit ScopedEnvironmentVariable(const char *name)
      : m_name(name),
        m_wasSet(qEnvironmentVariableIsSet(name)),
        m_oldValue(qgetenv(name))
  {
  }

  bool set(const QByteArray &value)
  {
    return qputenv(m_name, value);
  }

  ~ScopedEnvironmentVariable()
  {
    if (m_wasSet) {
      qputenv(m_name, m_oldValue);
    } else {
      qunsetenv(m_name);
    }
  }

private:
  const char *m_name;
  bool m_wasSet;
  QByteArray m_oldValue;
};

} // namespace

void CoreProcessTests::initTestCase()
{
  QDir dir;
  QVERIFY(dir.mkpath(m_settingsPath));

  QFile oldSettings(m_settingsFile);
  if (oldSettings.exists())
    oldSettings.remove();

  Settings::setSettingsFile(m_settingsFile);
  Settings::setStateFile(m_stateFile);
  Settings::setValue(Settings::Core::ProcessMode, Settings::ProcessMode::Desktop);
}

void CoreProcessTests::stop_terminatesDesktopChild()
{
  const auto childPath =
    QDir(QCoreApplication::applicationDirPath()).filePath(
      QStringLiteral("CoreProcessTestChild") + QStringLiteral(CORE_PROCESS_TEST_EXECUTABLE_SUFFIX));
  QVERIFY(QFile::exists(childPath));

  const auto ipcName = QStringLiteral("core-process-test-%1").arg(QUuid::createUuid().toString(QUuid::Id128));
  ServerConfig config;
  CoreProcess coreProcess(config, childPath, ipcName);
  coreProcess.setMode(Settings::CoreMode::Client);

  QSignalSpy processStateChangedSpy(&coreProcess, &CoreProcess::processStateChanged);
  QVERIFY(processStateChangedSpy.isValid());

  coreProcess.start(Settings::ProcessMode::Desktop);
  QCOMPARE(coreProcess.processState(), deskflow::core::ProcessState::Started);

  coreProcess.stop(Settings::ProcessMode::Desktop);
  QTRY_COMPARE(coreProcess.processState(), deskflow::core::ProcessState::Stopped);
}

void CoreProcessTests::stop_sendsGracefulIpcStop()
{
  const auto childPath =
      QDir(QCoreApplication::applicationDirPath()).filePath(
          QStringLiteral("CoreProcessTestChild") + QStringLiteral(CORE_PROCESS_TEST_EXECUTABLE_SUFFIX));
  QVERIFY(QFile::exists(childPath));

  const auto resultPath = QDir(m_settingsPath).filePath(QStringLiteral("graceful-stop-result"));
  QFile::remove(resultPath);
  QVERIFY(!QFile::exists(resultPath));

  ScopedEnvironmentVariable gracefulStopResult(kGracefulStopResultEnvironment);
  QVERIFY(gracefulStopResult.set(QFile::encodeName(resultPath)));
  const auto ipcName = QStringLiteral("core-process-test-%1").arg(QUuid::createUuid().toString(QUuid::Id128));
  ScopedEnvironmentVariable testIpcName(kIpcNameEnvironment);
  QVERIFY(testIpcName.set(ipcName.toUtf8()));

  ServerConfig config;
  CoreProcess coreProcess(config, childPath, ipcName);
  coreProcess.setMode(Settings::CoreMode::Client);

  QSignalSpy connectionStateChangedSpy(&coreProcess, &CoreProcess::connectionStateChanged);
  QVERIFY(connectionStateChangedSpy.isValid());

  coreProcess.start(Settings::ProcessMode::Desktop);
  QCOMPARE(coreProcess.processState(), deskflow::core::ProcessState::Started);
  QTRY_COMPARE(coreProcess.connectionState(), deskflow::core::ConnectionState::Connected);
  QVERIFY(!connectionStateChangedSpy.isEmpty());

  coreProcess.stop(Settings::ProcessMode::Desktop);
  QTRY_VERIFY(QFile::exists(resultPath));
  QTRY_COMPARE(coreProcess.processState(), deskflow::core::ProcessState::Stopped);

  QFile resultFile(resultPath);
  QVERIFY(resultFile.open(QIODevice::ReadOnly));
  QCOMPARE(resultFile.readAll(), QByteArray("graceful-stop"));
}

void CoreProcessTests::stop_killsDesktopChildIgnoringTerminate()
{
#ifdef Q_OS_WIN
  QSKIP("QProcess::terminate() does not use SIGTERM on Windows");
#else
  ScopedEnvironmentVariable ignoreTerminate(kIgnoreTerminateEnvironment);
  QVERIFY(ignoreTerminate.set("1"));

  const auto childPath =
      QDir(QCoreApplication::applicationDirPath()).filePath(
          QStringLiteral("CoreProcessTestChild") + QStringLiteral(CORE_PROCESS_TEST_EXECUTABLE_SUFFIX));
  QVERIFY(QFile::exists(childPath));

  const auto ipcName = QStringLiteral("core-process-test-%1").arg(QUuid::createUuid().toString(QUuid::Id128));
  ServerConfig config;
  CoreProcess coreProcess(config, childPath, ipcName);
  coreProcess.setMode(Settings::CoreMode::Client);

  QSignalSpy logLineSpy(&coreProcess, &CoreProcess::logLine);
  QVERIFY(logLineSpy.isValid());

  coreProcess.start(Settings::ProcessMode::Desktop);
  QCOMPARE(coreProcess.processState(), deskflow::core::ProcessState::Started);
  if (!logLineSpy.wait(2000)) {
    coreProcess.stop(Settings::ProcessMode::Desktop);
    QFAIL("test child did not report readiness");
  }
  QCOMPARE(logLineSpy.first().first().toString(), QStringLiteral("ready"));

  coreProcess.stop(Settings::ProcessMode::Desktop);
  QTRY_COMPARE(coreProcess.processState(), deskflow::core::ProcessState::Stopped);
#endif
}

QTEST_MAIN(CoreProcessTests)
