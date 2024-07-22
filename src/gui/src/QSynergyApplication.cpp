/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Symless Ltd.
 * Copyright (C) 2008 Volker Lanz (vl@fidra.de)
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

#include "QSynergyApplication.h"

#include "MainWindow.h"

#include <QtCore>
#include <QtGui>

QSynergyApplication::QSynergyApplication(int &argc, char **argv)
    : QApplication(argc, argv) {

  QFontDatabase::addApplicationFont(":/res/fonts/Arial.ttf");
  QFont Arial("Arial");
  Arial.setPixelSize(13);
  Arial.setStyleHint(QFont::SansSerif);
  setFont(Arial);

  // Setting the style to 'Fusion' seems to fix issues such as text being
  // rendered as black on black. This may not be the style we want long-term
  // but it does fix the style issues for now.
  setStyle("Fusion");
}

void QSynergyApplication::commitData(const QSessionManager &) const {
  foreach (QWidget *widget, topLevelWidgets()) {
    MainWindow *mainWindow = qobject_cast<MainWindow *>(widget);
    if (mainWindow)
      mainWindow->saveSettings();
  }
}
