/*
 * Deskflow -- mouse and keyboard sharing utility
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

#include "QProcessProxy.h"

namespace deskflow::gui::proxy {

void QProcessProxy::create()
{
  m_pProcess = std::make_unique<QProcess>();

  connect(m_pProcess.get(), &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
    Q_EMIT finished(exitCode, exitStatus);
  });

  connect(
      m_pProcess.get(), &QProcess::readyReadStandardOutput, //
      this, [this]() { Q_EMIT readyReadStandardOutput(); }
  );

  connect(
      m_pProcess.get(), &QProcess::readyReadStandardError, //
      this, [this]() { Q_EMIT readyReadStandardError(); }
  );
}

QProcessProxy::operator bool() const
{
  return m_pProcess.get();
}

void QProcessProxy::start(const QString &program, const QStringList &arguments)
{
  m_pProcess->start(program, arguments);
}

bool QProcessProxy::waitForStarted()
{
  return m_pProcess->waitForStarted();
}

QProcess::ProcessState QProcessProxy::state() const
{
  return m_pProcess->state();
}

void QProcessProxy::close()
{
  m_pProcess->close();
}

QString QProcessProxy::readAllStandardOutput()
{
  return m_pProcess->readAllStandardOutput();
}

QString QProcessProxy::readAllStandardError()
{
  return m_pProcess->readAllStandardError();
}

} // namespace deskflow::gui::proxy
