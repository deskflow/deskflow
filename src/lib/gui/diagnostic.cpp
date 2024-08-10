/*
 * synergy -- mouse and keyboard sharing utility
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

#include "diagnostic.h"

#include "config/ConfigScopes.h"

#include <QApplication>
#include <QProcess>
#include <qglobal.h>

namespace synergy::gui::diagnostic {

void restart() {
  QString program = QCoreApplication::applicationFilePath();
  QStringList arguments = QCoreApplication::arguments();

  // prevent infinite reset loop when env var set.
  arguments << "--no-reset";

  qInfo("launching new process: %s", qPrintable(program));
  QProcess::startDetached(program, arguments);

  qDebug("exiting current process");
  QApplication::exit();
}

void clearSettings(ConfigScopes &scopes, bool enableRestart) {
  qDebug("clearing settings");
  scopes.clear();

  // TODO: delete core config file, tls files, and legacy files.

  if (enableRestart) {
    qDebug("restarting");
    restart();
  } else {
    qDebug("skipping restart");
  }
}

} // namespace synergy::gui::diagnostic
