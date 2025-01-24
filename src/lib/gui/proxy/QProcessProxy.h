/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QObject>
#include <QProcess>

namespace deskflow::gui::proxy {

class QProcessProxy : public QObject
{
  Q_OBJECT

public:
  ~QProcessProxy() override = default;
  explicit virtual operator bool() const;
  virtual void create();
  virtual void start(const QString &program, const QStringList &arguments = {});
  virtual bool waitForStarted();
  virtual QProcess::ProcessState state() const;
  virtual void close();
  virtual QString readAllStandardOutput();
  virtual QString readAllStandardError();

signals:
  void finished(int exitCode, QProcess::ExitStatus exitStatus);
  void readyReadStandardOutput();
  void readyReadStandardError();

private:
  std::unique_ptr<QProcess> m_pProcess;
};

} // namespace deskflow::gui::proxy
