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
#include "ActivationNotifier.h"
#include "LicenseManager.h"
#include "QSynergyApplication.h"
#include "QUtility.h"

#include <QMessageBox>

SetupWizard::SetupWizard(MainWindow& mainWindow, bool startMain) :
    m_MainWindow(mainWindow),
    m_StartMain(startMain)
{
    setupUi(this);

#if defined(Q_OS_MAC)

    // the mac style needs a little more room because of the
    // graphic on the left.
    resize(600, 500);
    setMinimumSize(size());

    // additionally, we identified an issue with presenting dots, see SYNERGY-719
    duplicateSpaces();

#elif defined(Q_OS_WIN)

    // when areo is disabled on windows, the next/back buttons
    // are hidden (must be a qt bug) -- resizing the window
    // to +1 of the original height seems to fix this.
    // NOTE: calling setMinimumSize after this will break
    // it again, so don't do that.
    resize(size().width(), size().height() + 1);

#endif

    connect(m_pServerRadioButton, SIGNAL(toggled(bool)), m_MainWindow.m_pGroupServer, SLOT(setChecked(bool)));
    connect(m_pClientRadioButton, SIGNAL(toggled(bool)), m_MainWindow.m_pGroupClient, SLOT(setChecked(bool)));
}

SetupWizard::~SetupWizard()
{
}

bool SetupWizard::validateCurrentPage()
{
    QMessageBox message;
    message.setWindowTitle(tr("Setup Synergy"));
    message.setIcon(QMessageBox::Information);

    if (currentPage() == m_pNodePage)
    {
        bool result = m_pClientRadioButton->isChecked() ||
                 m_pServerRadioButton->isChecked();

        if (!result)
        {
            message.setText(tr("Please select an option."));
            message.exec();
            return false;
        }
    }

    return true;
}

void SetupWizard::accept()
{
    AppConfig& appConfig = m_MainWindow.appConfig();

    appConfig.setWizardHasRun();
    appConfig.saveSettings();

    QSettings& settings = m_MainWindow.settings();
    if (m_pServerRadioButton->isChecked())
    {
        settings.setValue("groupServerChecked", true);
        settings.setValue("groupClientChecked", false);
    }
    if (m_pClientRadioButton->isChecked())
    {
        settings.setValue("groupClientChecked", true);
        settings.setValue("groupServerChecked", false);
    }

    QWizard::accept();

    if (m_StartMain)
    {
        m_MainWindow.open();
    }
}

void SetupWizard::reject()
{
    QSynergyApplication::getInstance()->switchTranslator(m_MainWindow.appConfig().language());

    if (m_StartMain)
    {
        m_MainWindow.open();
    }

    QWizard::reject();
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
