/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2016 Synergy Ltd.
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

#include "gui/LicenseDisplay.h"

#include <QDialog>

namespace Ui {
class ActivationDialog;
}

class AppConfig;

class ActivationDialog : public QDialog {
  Q_OBJECT

public:
  ActivationDialog(
      QWidget *parent, AppConfig &appConfig, LicenseDisplay &licenseDisplay);
  ~ActivationDialog() override;

public slots:
  void reject();
  void accept();

protected:
  void refreshSerialKey();

private:
  Ui::ActivationDialog *ui;
  AppConfig *m_appConfig;
  LicenseDisplay &m_LicenseDisplay;
};
