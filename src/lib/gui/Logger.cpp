/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "Logger.h"

#include <QDateTime>
#include <QDir>
#include <QMessageBox>
#include <QTime>

#if defined(Q_OS_WIN)
#include <Windows.h>
#endif

namespace deskflow::gui {

const auto kForceDebugMessages = QStringList{
    QStringLiteral("No functional TLS backend was found"), QStringLiteral("No TLS backend is available"),
    QStringLiteral("QSslSocket::connectToHostEncrypted: TLS initialization failed"),
    QStringLiteral("Retrying to obtain clipboard."), QStringLiteral("Unable to obtain clipboard.")
};

QString printLine(FILE *out, const QString &type, const QString &message, const QString &fileLine = "")
{

  auto datetime = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-ddTHH:mm:ss"));
  auto logLine = QStringLiteral("[%1] %2: %3").arg(datetime, type, message);

  QTextStream stream(&logLine);
  if (!fileLine.isEmpty()) {
    stream << QStringLiteral("\n\t%1").arg(fileLine);
  }

  // We must return a non-terminated log line, but before returning,
  // stdout/stderr and Windows debug output all expect a terminated line.
  QString terminatedLogLine = logLine;
  QTextStream terminatedStream(&terminatedLogLine);
  terminatedStream << Qt::endl;

#if defined(Q_OS_WIN)
  // Debug output is viewable using either VS Code, Visual Studio, DebugView, or
  // DbgView++ (only one can be used at once). It's important to send output to
  // the debug output API, because it's difficult to view stdout and stderr from
  // a Windows GUI app.
  OutputDebugStringA(terminatedLogLine.toLocal8Bit().constData());
#else
  QTextStream outStream(out);
  outStream << terminatedLogLine;
#endif

  return logLine;
}

void Logger::loadEnvVars()
{
  m_debug = QVariant(qEnvironmentVariable("DESKFLOW_GUI_DEBUG")).toBool();
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
