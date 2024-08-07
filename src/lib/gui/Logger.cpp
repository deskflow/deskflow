/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Logger.h"

#include "string_utils.h"

#include <QDateTime>
#include <QDir>
#include <QMessageBox>
#include <QTime>

namespace synergy::gui {

Logger Logger::s_instance;

QString printLine(
    FILE *out, const QString &type, const QString &message,
    const QString &fileLine = "") {
  auto datetime = QDateTime::currentDateTime().toString("yyyy-MM-ddTHH:mm:ss");
  auto logLine = QString("[%1] %2: %3").arg(datetime).arg(type).arg(message);

  if (!fileLine.isEmpty()) {
    logLine += "\n\t" + fileLine;
  }

  auto logLineUtf = logLine.toUtf8();
  auto logLine_c = logLineUtf.constData();
  fprintf(out, "%s\n", logLine_c);
  fflush(out);
  return logLine;
}

void Logger::loadEnvVars() {
  const auto debugEnvVar = qEnvironmentVariable("SYNERGY_GUI_DEBUG");
  if (!debugEnvVar.isEmpty()) {
    m_debug = strToTrue(debugEnvVar);
  }

  const auto verboseEnvVar = qEnvironmentVariable("SYNERGY_GUI_VERBOSE");
  if (!verboseEnvVar.isEmpty()) {
    m_verbose = strToTrue(verboseEnvVar);
  }
}

void Logger::logVerbose(const QString &message) const {
  if (m_verbose) {
    printLine(stdout, "VERBOSE", message);
  }
}

void Logger::handleMessage(
    QtMsgType type, const QMessageLogContext &context, const QString &message) {

  QString typeString;
  auto out = stdout;
  switch (type) {
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

  const auto fileLine = QString("%1:%2").arg(context.file).arg(context.line);
  const auto logLine = printLine(out, typeString, message, fileLine);
  emit newLine(logLine);
}

} // namespace synergy::gui
