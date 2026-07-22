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

using namespace deskflow::gui;

namespace {

constexpr auto kIgnoreTerminateEnvironment = "DESKFLOW_CORE_PROCESS_TEST_IGNORE_TERMINATE";

class ScopedEnvironmentVariable
{
public:
  explicit ScopedEnvironmentVariable(const char *name)
      : m_name(name),
        m_wasSet(qEnvironmentVariableIsSet(name)),
        m_oldValue(qgetenv(name))
  {
  }

  bool set(const char *value)
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

  ServerConfig config;
  CoreProcess coreProcess(config, childPath);
  coreProcess.setMode(Settings::CoreMode::Client);

  QSignalSpy processStateChangedSpy(&coreProcess, &CoreProcess::processStateChanged);
  QVERIFY(processStateChangedSpy.isValid());

  coreProcess.start(Settings::ProcessMode::Desktop);
  QCOMPARE(coreProcess.processState(), deskflow::core::ProcessState::Started);

  coreProcess.stop(Settings::ProcessMode::Desktop);
  QTRY_COMPARE(coreProcess.processState(), deskflow::core::ProcessState::Stopped);
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

  ServerConfig config;
  CoreProcess coreProcess(config, childPath);
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
