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
