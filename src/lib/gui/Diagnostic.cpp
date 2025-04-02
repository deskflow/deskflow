/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "Diagnostic.h"

#include "common/Settings.h"

#include <QCoreApplication>
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
  QCoreApplication::exit();
}

void clearSettings(bool enableRestart)
{
  qDebug("clearing settings");
  Settings::proxy().clear();

  // save but do not emit saving signal which will prevent the current state of
  // the app config and server configs from being applied.
  Settings::save(false);

  auto profileDir = QDir(Settings::settingsPath());
  qDebug("removing profile dir: %s", qPrintable(profileDir.absolutePath()));
  profileDir.removeRecursively();

#ifdef Q_OS_WIN
  if (Settings::isNativeMode()) {
    // make a new empty portable settings file
    if (profileDir.mkpath(Settings::settingsPath())) {
      QFile file(Settings::settingsFile());
      file.open(QIODevice::WriteOnly);
      file.write(" ", 1);
      file.close();
    }
  }
#endif

  if (enableRestart) {
    qDebug("restarting");
    restart();
  } else {
    qDebug("skipping restart");
  }
}

} // namespace deskflow::gui::diagnostic
