/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "Logger.h"

#include "string_utils.h"

#include <QDateTime>
#include <QDir>
#include <QMessageBox>
#include <QTime>

#if defined(Q_OS_WIN)
#include <Windows.h>
#endif

namespace deskflow::gui {

const auto kForceDebugMessages = QStringList{
    "No functional TLS backend was found", "No TLS backend is available",
    "QSslSocket::connectToHostEncrypted: TLS initialization failed", "Retrying to obtain clipboard.",
    "Unable to obtain clipboard."
};

Logger Logger::s_instance;

QString printLine(FILE *out, const QString &type, const QString &message, const QString &fileLine = "")
{

  auto datetime = QDateTime::currentDateTime().toString("yyyy-MM-ddTHH:mm:ss");
  auto logLine = QString("[%1] %2: %3").arg(datetime, type, message);

  QTextStream stream(&logLine);
  stream << Qt::endl;

  if (!fileLine.isEmpty()) {
    stream << "\t" + fileLine << Qt::endl;
  }

  auto logLineBytes = logLine.toUtf8();
  auto logLine_c = logLineBytes.constData();

#if defined(Q_OS_WIN)
  // Debug output is viewable using either VS Code, Visual Studio, DebugView, or
  // DbgView++ (only one can be used at once). It's important to send output to
  // the debug output API, because it's difficult to view stdout and stderr from
  // a Windows GUI app.
  OutputDebugStringA(logLine_c);
#else
  fprintf(out, "%s", logLine_c);
  fflush(out);
#endif

  return logLine;
}

void Logger::loadEnvVars()
{
  const auto debugEnvVar = qEnvironmentVariable("DESKFLOW_GUI_DEBUG");
  if (!debugEnvVar.isEmpty()) {
    m_debug = strToTrue(debugEnvVar);
  }

  const auto verboseEnvVar = qEnvironmentVariable("DESKFLOW_GUI_VERBOSE");
  if (!verboseEnvVar.isEmpty()) {
    m_verbose = strToTrue(verboseEnvVar);
  }
}

void Logger::logVerbose(const QString &message) const
{
  if (m_verbose) {
    printLine(stdout, "VERBOSE", message);
  }
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
    typeString = "DEBUG";
    if (!m_debug) {
      return;
    }
    break;
  case QtInfoMsg:
    typeString = "INFO";
    break;
  case QtWarningMsg:
    typeString = "WARNING";
    out = stderr;
    break;
  case QtCriticalMsg:
    typeString = "CRITICAL";
    out = stderr;
    break;
  case QtFatalMsg:
    typeString = "FATAL";
    out = stderr;
    break;
  }

  const auto logLine = printLine(out, typeString, message, fileLine);
  Q_EMIT newLine(logLine);
}

} // namespace deskflow::gui
