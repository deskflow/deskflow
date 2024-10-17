/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Symless Ltd.
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

#include "gui/core/CoreTool.h"

#include "CommandProcess.h"

#include <QCoreApplication>
#include <QDir>
#include <QProcess>
#include <QtGlobal>

static const char kCoreBinary[] = LEGACY_BINARY_NAME;

QString CoreTool::getProfileDir() const
{
  QStringList args("--get-profile-dir");
  return QDir::cleanPath(run(args));
}

QString CoreTool::getInstalledDir() const
{
  QStringList args("--get-installed-dir");
  return QDir::cleanPath(run(args));
}

QString CoreTool::getArch() const
{
  QStringList args("--get-arch");
  return run(args);
}

QString CoreTool::run(const QStringList &args, const QString &input) const
{
  QString program(QCoreApplication::applicationDirPath() + "/" + kCoreBinary);

  CommandProcess commandProcess(program, args, input);
  return commandProcess.run();
}
