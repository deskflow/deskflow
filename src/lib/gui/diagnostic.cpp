/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "diagnostic.h"

#include "config/ConfigScopes.h"
#include "paths.h"

#include <QApplication>
#include <QDir>
#include <QProcess>

namespace deskflow::gui::diagnostic {

void restart()
{
  QString program = QCoreApplication::applicationFilePath();
  QStringList arguments = QCoreApplication::arguments();

  // prevent infinite reset loop when env var set.
  arguments << "--no-reset";

  qInfo("launching new process: %s", qPrintable(program));
  QProcess::startDetached(program, arguments);

  qDebug("exiting current process");
  QApplication::exit();
}

void clearSettings(ConfigScopes &scopes, bool enableRestart)
{
  qDebug("clearing settings");
  scopes.clear();

  // save but do not emit saving signal which will prevent the current state of
  // the app config and server configs from being applied.
  scopes.save(false);

  auto configDir = paths::configDir();
  qDebug("removing config dir: %s", qPrintable(configDir.absolutePath()));
  configDir.removeRecursively();

  auto profileDir = paths::coreProfileDir();
  qDebug("removing profile dir: %s", qPrintable(profileDir.absolutePath()));
  profileDir.removeRecursively();

  if (enableRestart) {
    qDebug("restarting");
    restart();
  } else {
    qDebug("skipping restart");
  }
}

} // namespace deskflow::gui::diagnostic
