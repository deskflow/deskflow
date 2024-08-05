/*
 * synergy -- mouse and keyboard sharing utility
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

#include "gui/styles.h"
#include "validators/ScreenNameValidator.h"
#include "validators/ValidationError.h"

using namespace synergy::gui;

SetupWizard::SetupWizard(AppConfig &appConfig) : m_appConfig(appConfig) {
  setupUi(this);

  m_pLabelError->setStyleSheet(kStyleErrorActiveLabel);

  m_pLineEditName->setText(appConfig.screenName());
  m_pLineEditName->setValidator(new validators::ScreenNameValidator(
      m_pLineEditName, new validators::ValidationError(this, m_pLabelError)));

  connect(m_pButtonApply, &QPushButton::clicked, this, &SetupWizard::accept);
  connect(
      m_pLineEditName, &QLineEdit::textChanged, this,
      &SetupWizard::onLineEditNameChanged);
}

void SetupWizard::accept() {
  m_appConfig.setWizardHasRun();
  m_appConfig.setScreenName(m_pLineEditName->text());
  QDialog::accept();
}

void SetupWizard::onLineEditNameChanged(const QString &error) {
  m_pButtonApply->setEnabled(m_pLineEditName->hasAcceptableInput());
}

void SetupWizard::reject() {
  QDialog::reject();
  QApplication::exit();
}
