/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2014 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "CommandProcess.h"

#include <QProcess>

CommandProcess::CommandProcess(QString command, QStringList arguments, QString input)
    : m_Command(command),
      m_Arguments(arguments),
      m_Input(input)
{
}

QString CommandProcess::run()
{
  QProcess process;
  process.setReadChannel(QProcess::StandardOutput);
  process.start(m_Command, m_Arguments);
  bool success = process.waitForStarted();

  QString output;
  QString error;
  if (success) {
    if (!m_Input.isEmpty()) {
      process.write(m_Input.toStdString().c_str());
    }

    if (process.waitForFinished()) {
      output = process.readAllStandardOutput().trimmed();
      error = process.readAllStandardError().trimmed();
    }
  }

  if (int code = process.exitCode(); !success || code != 0) {
    qFatal(
        "command failed: %s %s\ncode: %d\nerror: %s", qUtf8Printable(m_Command), qUtf8Printable(m_Arguments.join(" ")),
        code, error.isEmpty() ? "none" : qUtf8Printable(error)
    );
  }

  Q_EMIT finished();

  return output;
}
