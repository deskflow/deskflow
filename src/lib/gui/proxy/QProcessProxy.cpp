/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
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
