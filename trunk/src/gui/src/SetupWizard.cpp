/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
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
#include "QSynergyApplication.h"
#include "QUtility.h"
#include "PremiumAuth.h"

#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>

#define PREMIUM_REGISTER_URL "http://synergy-foss.org/donate/?source=gui-wizard"

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
	QString premiumEmail = appConfig.premiumEmail();
	if (!premiumEmail.isEmpty())
	{
		m_pRadioButtonPremiumLogin->setChecked(true);
		m_pLineEditPremiumEmail->setText(premiumEmail);
	}
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
	else if (currentPage() == m_pPremiumUserPage)
	{
		if (m_pRadioButtonPremiumLogin->isChecked())
		{
			if (m_pLineEditPremiumEmail->text().isEmpty() ||
				m_pLineEditPremiumPassword->text().isEmpty())
			{
				message.setText(tr("Please enter your email address and password."));
				message.exec();
				return false;
			}
			else if (!isPremiumLoginValid(message))
			{
				return false;
			}
		}
		else if (m_pRadioButtonPremiumRegister->isChecked())
		{
			const QUrl url(QString(PREMIUM_REGISTER_URL));
			QDesktopServices::openUrl(url);
			m_pRadioButtonPremiumLogin->setChecked(true);
			return false;
		}
		else if (m_pRadioButtonPremiumLater->isChecked())
		{
			return true;
		}
		else {
			message.setText(tr("Please select an option."));
			message.exec();
			return false;
		}
	}
	else if (currentPage() == m_pCryptoPage)
	{
		if (m_pCheckBoxEnableCrypto->isChecked())
		{
			if (m_pLineEditCryptoPassword1->text().isEmpty())
			{
				message.setText(tr("Encryption password required."));
				message.exec();
				return false;
			}

			if (m_pLineEditCryptoPassword1->text() != m_pLineEditCryptoPassword2->text())
			{
				message.setText(tr("Encryption password and confirmation do not match."));
				message.exec();
				return false;
			}
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

	appConfig.setCryptoEnabled(m_pCheckBoxEnableCrypto->isChecked());
	appConfig.setCryptoPass(m_pLineEditCryptoPassword1->text());
	appConfig.setLanguage(m_pComboLanguage->itemData(m_pComboLanguage->currentIndex()).toString());
	appConfig.setPremiumEmail(m_pLineEditPremiumEmail->text());

	if (!m_pRadioButtonPremiumLogin->isChecked())
	{
		appConfig.setPremiumToken("");
	}
	else
	{
		QString mac = getFirstMacAddress();
		QString hashSrc = m_pLineEditPremiumEmail->text() + mac;
		QString hashResult = hash(hashSrc);
		appConfig.setPremiumToken(hashResult);
	}

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

	settings.sync();

	if (m_StartMain)
	{
		m_MainWindow.start();
	}

	QWizard::accept();
}

void SetupWizard::reject()
{
	QSynergyApplication::getInstance()->switchTranslator(m_MainWindow.appConfig().language());

	if (m_StartMain)
	{
		m_MainWindow.start();
	}

	QWizard::reject();
}

void SetupWizard::on_m_pCheckBoxEnableCrypto_stateChanged(int )
{
	bool enabled = m_pCheckBoxEnableCrypto->isChecked();
	m_pLineEditCryptoPassword1->setEnabled(enabled);
	m_pLineEditCryptoPassword2->setEnabled(enabled);
	if (!enabled)
	{
		m_pLineEditCryptoPassword1->clear();
		m_pLineEditCryptoPassword2->clear();
	}
}

void SetupWizard::on_m_pComboLanguage_currentIndexChanged(int index)
{
	QString ietfCode = m_pComboLanguage->itemData(index).toString();
	QSynergyApplication::getInstance()->switchTranslator(ietfCode);
}

void SetupWizard::on_m_pRadioButtonPremiumLogin_toggled(bool checked)
{
	m_pLineEditPremiumEmail->setEnabled(checked);
	m_pLineEditPremiumPassword->setEnabled(checked);
}

bool SetupWizard::isPremiumLoginValid(QMessageBox& message)
{
	QString email = m_pLineEditPremiumEmail->text();
	QString password = m_pLineEditPremiumPassword->text();

	PremiumAuth auth;
	QString responseJson = auth.request(email, password);

	if (responseJson.trimmed() == "") {
		message.setText(tr("Login failed, could not communicate with server."));
		message.exec();
		return false;
	}

	QRegExp resultRegex(".*\"result\".*:.*(true|false).*");
	if (resultRegex.exactMatch(responseJson)) {
		QString boolString = resultRegex.cap(1);
		if (boolString == "true") {
			return true;
		}
		else if (boolString == "false") {
			message.setText(tr("Login failed, invalid email or password."));
			message.exec();
			return false;
		}
	}
	else {
		QRegExp errorRegex(".*\"error\".*:.*\"(.+)\".*");
		if (errorRegex.exactMatch(responseJson)) {

			// replace "\n" with real new lines.
			QString error = errorRegex.cap(1).replace("\\n", "\n");

			message.setText(tr("Login failed, an error occurred.\n\n%1").arg(error));
			message.exec();
			return false;
		}
	}

	message.setText(tr("Login failed, an error occurred.\n\nServer response:\n\n%1").arg(responseJson));
	message.exec();
	return false;
}
