/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Symless Ltd.
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
