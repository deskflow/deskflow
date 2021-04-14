/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
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
#include "MainWindow.h"
#include "validators/ScreenNameValidator.h"

SetupWizard::SetupWizard(MainWindow& mainWindow) :
   m_MainWindow(mainWindow)
{
   setupUi(this);

   m_pLineEditName->setText(m_MainWindow.appConfig().screenName());
   m_pLineEditName->setValidator(new validators::ScreenNameValidator(m_pLineEditName, label_ErrorMessage));

   connect(m_pButtonApply, SIGNAL(clicked()), this, SLOT(accept()));
   connect(m_pLineEditName, SIGNAL(textEdited(QString)), this, SLOT(onNameChanged()));
}

void SetupWizard::accept()
{
   AppConfig& appConfig = m_MainWindow.appConfig();

   appConfig.setWizardHasRun();
   appConfig.setScreenName(m_pLineEditName->text());
   appConfig.saveSettings();

   m_MainWindow.open();
   QDialog::accept();
}

void SetupWizard::onNameChanged()
{
   m_pButtonApply->setEnabled(label_ErrorMessage->text().isEmpty());
}

void SetupWizard::reject()
{
   QDialog::reject();
   QApplication::exit();
}
