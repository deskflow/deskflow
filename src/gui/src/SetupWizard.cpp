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

SetupWizard::SetupWizard(MainWindow& mainWindow) :
    m_MainWindow(mainWindow)
{
    setupUi(this);

#if defined(Q_OS_MAC)
    // we identified an issue with presenting dots, see SYNERGY-719
    duplicateSpaces();
#endif
}

SetupWizard::~SetupWizard()
{
}

void SetupWizard::accept()
{
    AppConfig& appConfig = m_MainWindow.appConfig();

    appConfig.setWizardHasRun();
    appConfig.saveSettings();

    QDialog::accept();
    m_MainWindow.open();
}

void SetupWizard::reject()
{
    m_MainWindow.open();
    QDialog::reject();
}

#if defined(Q_OS_MAC)
void SetupWizard::duplicateSpaces()
{
    auto list = this->findChildren<QLabel *>();
    foreach(QLabel *l, list) {
        if (l->wordWrap()) {
           l->setText(l->text().replace(". ", ".  "));
           l->setText(l->text().replace(", ", ",  "));
        }
    }
}
#endif // defined(Q_OS_MAC)
