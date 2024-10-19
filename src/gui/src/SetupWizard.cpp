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

#include "SetupWizard.h"
#include "ui_SetupWizard.h"

#include "gui/styles.h"
#include "gui/validators/ScreenNameValidator.h"
#include "gui/validators/ValidationError.h"

using namespace deskflow::gui;

SetupWizard::SetupWizard(AppConfig &appConfig) : ui{std::make_unique<Ui::SetupWizard>()}, m_appConfig(appConfig)
{
  ui->setupUi(this);

  setWindowTitle(QString("Setup %1").arg(DESKFLOW_APP_NAME));

  ui->m_pLabelError->setStyleSheet(kStyleErrorActiveLabel);

  ui->m_pLineEditName->setText(appConfig.screenName());
  ui->m_pLineEditName->setValidator(
      new validators::ScreenNameValidator(ui->m_pLineEditName, new validators::ValidationError(this, ui->m_pLabelError))
  );

  connect(ui->m_pButtonApply, &QPushButton::clicked, this, &SetupWizard::accept);
  connect(ui->m_pLineEditName, &QLineEdit::textChanged, this, &SetupWizard::nameChanged);
}

SetupWizard::~SetupWizard() = default;

void SetupWizard::accept()
{
  m_appConfig.setWizardHasRun();
  m_appConfig.setScreenName(ui->m_pLineEditName->text());
  QDialog::accept();
}

void SetupWizard::nameChanged(const QString &error)
{
  ui->m_pButtonApply->setEnabled(ui->m_pLineEditName->hasAcceptableInput());
}

void SetupWizard::reject()
{
  QDialog::reject();
  QApplication::exit();
}
