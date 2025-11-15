/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "Logger.h"
#include "common/Settings.h"

#include <QDateTime>
#include <QDir>
#include <QMessageBox>
#include <QTime>

#if defined(Q_OS_WIN)
#include <Windows.h>
#endif

using namespace Qt::StringLiterals;

namespace deskflow::gui {

const auto kForceDebugMessages = QStringList{
    u"No functional TLS backend was found"_s, u"No TLS backend is available"_s,
    u"QSslSocket::connectToHostEncrypted: TLS initialization failed"_s, u"Retrying to obtain clipboard."_s,
    u"Unable to obtain clipboard."_s
};

QString printLine(FILE *out, const QString &type, const QString &message, const QString &fileLine = {})
{
  const auto datetime = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
  auto logLine = u"[%1] %2: %3"_s.arg(datetime, type, message);

  if (!fileLine.isEmpty()) {
    logLine.append(u"\n\t%1"_s.arg(fileLine));
  }

  // We must return a non-terminated log line, but before returning,
  // stdout/stderr and Windows debug output all expect a terminated line.
  const auto terminatedLogLine = u"%1\n"_s.arg(logLine);

#if defined(Q_OS_WIN)
  // Debug output is viewable using either VS Code, Visual Studio, DebugView, or
  // DbgView++ (only one can be used at once). It's important to send output to
  // the debug output API, because it's difficult to view stdout and stderr from
  // a Windows GUI app.
  OutputDebugStringA(terminatedLogLine.toLocal8Bit().constData());
#else
  QTextStream(out) << terminatedLogLine;
#endif

  return logLine;
}

void Logger::handleMessage(const QtMsgType type, const QString &fileLine, const QString &message)
{
  auto mutatedType = type;
  if (kForceDebugMessages.contains(message)) {
    mutatedType = QtDebugMsg;
  }

  QString typeString;
  auto out = stdout;
  switch (mutatedType) {
  case QtDebugMsg:
    if (!m_guiDebug)
      return;
    typeString = u"DEBUG"_s;
    break;
  case QtInfoMsg:
    typeString = u"INFO"_s;
    break;
  case QtWarningMsg:
    typeString = u"WARNING"_s;
    out = stderr;
    break;
  case QtCriticalMsg:
    typeString = u"CRITICAL"_s;
    out = stderr;
    break;
  case QtFatalMsg:
    typeString = u"FATAL"_s;
    out = stderr;
    break;
  }

  const auto logLine = printLine(out, typeString, message, fileLine);
  Q_EMIT newLine(logLine);
}

Logger::Logger()
{
  m_guiDebug = Settings::value(Settings::Log::GuiDebug).toBool();
  connect(Settings::instance(), &Settings::settingsChanged, this, &Logger::settingChanged);
}

Logger::~Logger()
{
  disconnect(Settings::instance(), &Settings::settingsChanged, this, &Logger::settingChanged);
}

void Logger::settingChanged(const QString &key)
{
  if (key != Settings::Log::GuiDebug)
    return;
  m_guiDebug = Settings::value(Settings::Log::GuiDebug).toBool();
}

} // namespace deskflow::gui
