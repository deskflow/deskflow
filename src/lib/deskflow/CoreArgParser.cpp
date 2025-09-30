/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "CoreArgParser.h"
#include "CoreArgs.h"
#include "VersionInfo.h"
#include "common/Constants.h"
#include "common/ExitCodes.h"
#include "common/Settings.h"
#include "deskflow/ProtocolTypes.h"

const QString CoreArgParser::s_appName = QStringLiteral("%1-core").arg(kAppId);
const QString CoreArgParser::s_headerText = QStringLiteral("%1: %2\n").arg(s_appName, kDisplayVersion);

CoreArgParser::CoreArgParser(const QStringList &args)
{
  m_parser.setApplicationDescription(kAppDescription);
  m_parser.addPositionalArgument("coremode", "The mode to start in either: server or client", "coremode");

  m_parser.addOptions(CoreArgs::options);
  m_parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
  m_parser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsOptions);
  m_parser.parse(args);

  m_helpText = m_parser.helpText().replace("<executable_name>", s_appName);
  m_helpText.replace("[options] coremode", "coremode [options]");
}

void CoreArgParser::parse()
{
  auto posArgs = m_parser.positionalArguments();
  if (posArgs.isEmpty()) {
    showHelpText();
    exit(s_exitSuccess);
  }

  const QString mode = posArgs.takeFirst();
  m_serverMode = (mode.compare("server", Qt::CaseInsensitive) == 0);
  m_clientMode = (mode.compare("client", Qt::CaseInsensitive) == 0);

  if ((!m_clientMode && !m_serverMode) || mode.isEmpty()) {
    showHelpText();
    exit(s_exitSuccess);
  }

  if (m_parser.isSet(CoreArgs::configOption)) {
    Settings::setSettingFile(m_parser.value(CoreArgs::configOption));
  }

  if (m_parser.isSet(CoreArgs::interfaceOption)) {
    Settings::setValue(Settings::Core::Interface, m_parser.value(CoreArgs::interfaceOption));
  }

  if (m_parser.isSet(CoreArgs::portOption)) {
    Settings::setValue(Settings::Core::Port, m_parser.value(CoreArgs::portOption));
  }

  if (m_parser.isSet(CoreArgs::nameOption)) {
    Settings::setValue(Settings::Core::ScreenName, m_parser.value(CoreArgs::nameOption));
  }

  if (m_parser.isSet(CoreArgs::logLevelOption)) {
    Settings::setValue(Settings::Log::Level, Settings::logLevelToInt(m_parser.value(CoreArgs::logLevelOption)));
  }
}

[[noreturn]] void CoreArgParser::showHelpText() const
{
  QTextStream(stdout) << helpText();
  exit(s_exitSuccess);
}

QString CoreArgParser::helpText() const
{
  return QStringLiteral("%1%2").arg(s_headerText, m_helpText);
}

QString CoreArgParser::versionText() const
{
  const static auto vString = QStringLiteral("%1 v%2, protocol v%3.%4\n%5\n");
  return vString.arg(
      s_appName, kDisplayVersion, QString::number(kProtocolMajorVersion), QString::number(kProtocolMinorVersion),
      kCopyright
  );
}

QString CoreArgParser::errorText() const
{
  return m_parser.errorText();
}

bool CoreArgParser::help() const
{
  return m_parser.isSet(CoreArgs::helpOption);
}

bool CoreArgParser::version() const
{
  return m_parser.isSet(CoreArgs::versionOption);
}

bool CoreArgParser::serverMode() const
{
  return m_serverMode;
}

bool CoreArgParser::clientMode() const
{
  return m_clientMode;
}
