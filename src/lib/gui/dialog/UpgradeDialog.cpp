/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2022 Symless Ltd.
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

#include "UpgradeDialog.h"

#include "gui/constants.h"

#include <QtCore>
#include <QtGui>

UpgradeDialog::UpgradeDialog(QWidget *parent) : QMessageBox(parent) {
  setWindowTitle("Upgrade to access this feature");
  addButton("Cancel", QMessageBox::RejectRole);
  addButton("Upgrade", QMessageBox::AcceptRole);
}

void UpgradeDialog::showDialog(const QString &text) {
  setText(text);

  if (exec() == QMessageBox::Accepted) {
    QDesktopServices::openUrl(QUrl(synergy::gui::kUrlUpgrade));
  }
}
