/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Symless Ltd.
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

#include <QDialog>
#include <QObject>

class MainWindow;

namespace Ui {
class SetupWizard;
}

class AppConfig;

namespace Ui {
class SetupWizard;
}

class SetupWizard : public QDialog
{
  Q_OBJECT

public:
  explicit SetupWizard(AppConfig &appConfig);
  ~SetupWizard();

protected:
  void accept() override;
  void reject() override;

private:
  void nameChanged(const QString &error);

  std::unique_ptr<Ui::SetupWizard> ui;
  AppConfig &m_appConfig;
};
