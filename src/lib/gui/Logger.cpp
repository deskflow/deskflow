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

namespace deskflow::gui {

const auto kForceDebugMessages = QStringList{
    QStringLiteral("No functional TLS backend was found"), QStringLiteral("No TLS backend is available"),
    QStringLiteral("QSslSocket::connectToHostEncrypted: TLS initialization failed"),
    QStringLiteral("Retrying to obtain clipboard."), QStringLiteral("Unable to obtain clipboard.")
};

QString printLine(FILE *out, const QString &type, const QString &message, const QString &fileLine = {})
{
  const auto datetime = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
  auto logLine = QStringLiteral("[%1] %2: %3").arg(datetime, type, message);

  if (!fileLine.isEmpty()) {
    logLine.append(QStringLiteral("\n\t%1").arg(fileLine));
  }

  // We must return a non-terminated log line, but before returning,
  // stdout/stderr and Windows debug output all expect a terminated line.
  const auto terminatedLogLine = QStringLiteral("%1\n").arg(logLine);

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
    if (!Settings::value(Settings::Log::GuiDebug).toBool())
      return;
    typeString = "DEBUG";
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
