/*
 * barrier -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2016 Symless Ltd.
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
#include <stdexcept>

CommandProcess::CommandProcess(QString cmd, QStringList arguments, QString input) :
    m_Command(cmd),
    m_Arguments(arguments),
    m_Input(input)
{
}

QString CommandProcess::run()
{
    QProcess process;
    QString standardOutput, standardError;
    process.setReadChannel(QProcess::StandardOutput);
    process.start(m_Command, m_Arguments);
    bool success = process.waitForStarted();

    if (success)
    {
        if (!m_Input.isEmpty()) {
            process.write(m_Input.toLocal8Bit());
        }

        if (process.waitForFinished()) {
            standardOutput = QString::fromLocal8Bit(process.readAllStandardOutput().trimmed());
            standardError = QString::fromLocal8Bit(process.readAllStandardError().trimmed());
        }
    }

    int code = process.exitCode();
    if (!standardError.isEmpty() || !success || code != 0)
    {
        throw std::runtime_error(
            std::string(
                QString("Code: %1\nError: %2")
                    .arg(process.exitCode())
                    .arg(standardError.isEmpty() ? "Unknown" : standardError)
                .toLocal8Bit().constData()));
    }

    emit finished();

    return standardOutput;
}
