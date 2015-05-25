/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
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
#include "WebClient.h"
#include "EditionType.h"
#include "QSynergyApplication.h"
#include "QUtility.h"

#include <QMessageBox>

SetupWizard::SetupWizard(MainWindow& mainWindow, bool startMain) :
	m_MainWindow(mainWindow),
	m_StartMain(startMain),
	m_Edition(Unknown)
{
	setupUi(this);
	m_pPluginPage = new PluginWizardPage(mainWindow.appConfig());
	addPage(m_pPluginPage);

#if defined(Q_OS_MAC)

	// the mac style needs a little more room because of the
	// graphic on the left.
	resize(600, 500);
	setMinimumSize(size());

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

	m_Locale.fillLanguageComboBox(m_pComboLanguage);
	setIndexFromItemData(m_pComboLanguage, m_MainWindow.appConfig().language());
	AppConfig& appConfig = m_MainWindow.appConfig();

	m_pLineEditEmail->setText(appConfig.activateEmail());

}

SetupWizard::~SetupWizard()
{
}

bool SetupWizard::validateCurrentPage()
{	
	QMessageBox message;
	message.setWindowTitle(tr("Setup Synergy"));
	message.setIcon(QMessageBox::Information);

	if (currentPage() == m_pActivatePage)
	{
		if (m_pRadioButtonActivate->isChecked()) {
			if (m_pLineEditEmail->text().isEmpty() ||
				m_pLineEditPassword->text().isEmpty()) {
				message.setText(tr("Please enter your email address and password."));
				message.exec();
				return false;
			}
			else {
				WebClient webClient;
				m_Edition = webClient .getEdition(
					m_pLineEditEmail->text(),
					m_pLineEditPassword->text(),
					message,
					this);

				if (m_Edition == Unknown) {
					return false;
				}
				else {
					m_pPluginPage->setEmail(m_pLineEditEmail->text());
					m_pPluginPage->setPassword(m_pLineEditPassword->text());
					return true;
				}
			}
		}
		else {
			return true;
		}
	}
	else if (currentPage() == m_pNodePage)
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

void SetupWizard::changeEvent(QEvent* event)
{
	if (event != 0)
	{
		switch (event->type())
		{
		case QEvent::LanguageChange:
			{
				m_pComboLanguage->blockSignals(true);
				retranslateUi(this);
				m_pComboLanguage->blockSignals(false);
				break;
			}

		default:
			QWizard::changeEvent(event);
		}
	}
}

void SetupWizard::accept()
{
	AppConfig& appConfig = m_MainWindow.appConfig();

	appConfig.setLanguage(m_pComboLanguage->itemData(m_pComboLanguage->currentIndex()).toString());

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

	if (m_pRadioButtonActivate->isChecked()) {
		appConfig.setActivateEmail(m_pLineEditEmail->text());
		QString mac = getFirstMacAddress();
		QString hashSrc = m_pLineEditEmail->text() + mac;
		QString hashResult = hash(hashSrc);
		appConfig.setUserToken(hashResult);
		appConfig.setEdition(m_Edition);
	}
	m_MainWindow.setEdition(m_Edition);
	m_MainWindow.updateLocalFingerprint();

	settings.sync();

	QWizard::accept();

	if (m_StartMain)
	{
		m_MainWindow.updateZeroconfService();
		m_MainWindow.open();
	}
}

void SetupWizard::reject()
{
	QSynergyApplication::getInstance()->switchTranslator(m_MainWindow.appConfig().language());

	if (m_StartMain)
	{
		m_MainWindow.setEdition(m_Edition);
		m_MainWindow.open();
	}

	QWizard::reject();
}

void SetupWizard::on_m_pComboLanguage_currentIndexChanged(int index)
{
	QString ietfCode = m_pComboLanguage->itemData(index).toString();
	QSynergyApplication::getInstance()->switchTranslator(ietfCode);
}

void SetupWizard::on_m_pRadioButtonSkip_toggled(bool checked)
{
	if (checked) {
		m_pLineEditEmail->setEnabled(false);
		m_pLineEditPassword->setEnabled(false);
	}
}

void SetupWizard::on_m_pRadioButtonActivate_toggled(bool checked)
{
	if (checked) {
		m_pLineEditEmail->setEnabled(true);
		m_pLineEditPassword->setEnabled(true);
	}
}
