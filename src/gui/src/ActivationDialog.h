/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2016 Symless Ltd.
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

#include "gui/license/LicenseHandler.h"

#include <QDialog>

namespace Ui {
class ActivationDialog;
}

class AppConfig;

class ActivationDialog : public QDialog {
  Q_OBJECT

public:
  ActivationDialog(
      QWidget *parent, AppConfig &appConfig, LicenseHandler &licenseHandler);
  ~ActivationDialog() override;

  class ActivationMessageError : public std::runtime_error {
  public:
    ActivationMessageError()
        : std::runtime_error("could not show activation message") {}
  };

public slots:
  void reject() override;
  void accept() override;

protected:
  void refreshSerialKey();

private:
  void showResultDialog(LicenseHandler::ChangeSerialKeyResult result);
  void showSuccessDialog();
  void showErrorDialog(const QString &message);

  Ui::ActivationDialog *m_ui;
  AppConfig *m_pAppConfig;
  LicenseHandler &m_licenseHandler;
};
