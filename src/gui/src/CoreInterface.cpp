/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015-2016 Symless Ltd.
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

#include "CoreInterface.h"

#include "CommandProcess.h"

#include <QCoreApplication>
#include <QDir>
#include <QProcess>
#include <QtGlobal>
#include <qdir.h>

static const char kCoreBinary[] = "syntool";

#ifdef Q_WS_WIN
static const char kSerialKeyFilename[] = "Synergy.subkey";
#else
static const char kSerialKeyFilename[] = ".synergy.subkey";
#endif

QString CoreInterface::getProfileDir() {
  QStringList args("--get-profile-dir");
  return QDir::cleanPath(run(args));
}

QString CoreInterface::getInstalledDir() {
  QStringList args("--get-installed-dir");
  return QDir::cleanPath(run(args));
}

QString CoreInterface::getArch() {
  QStringList args("--get-arch");
  return run(args);
}

QString CoreInterface::getSerialKeyFilePath() {
  auto filename = getProfileDir() + QDir::separator() + kSerialKeyFilename;
  return QDir::cleanPath(filename);
}

QString
CoreInterface::run(const QStringList &args, const QString &input) const {
  QString program(QCoreApplication::applicationDirPath() + "/" + kCoreBinary);

  CommandProcess commandProcess(program, args, input);
  return commandProcess.run();
}
