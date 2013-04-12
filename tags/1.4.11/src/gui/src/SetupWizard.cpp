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

#include <QMessageBox>
#include <iostream>

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

	connect(this, SIGNAL(finished(int)), this, SLOT(handleFinished()));
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
	else if (currentPage() == m_pCryptoPage)
	{
		QString modeText = m_pComboCryptoMode->currentText();
		if (modeText.isEmpty())
		{
			message.setText(tr("Encryption mode required."));
			message.exec();
			return false;
		}

		if (parseCryptoMode(modeText) != Disabled)
		{
			if (m_pLineEditCryptoPass->text().isEmpty())
			{
				message.setText(tr("Encryption password required."));
				message.exec();
				return false;
			}

			if (m_pLineEditCryptoPass->text() != m_pLineEditCryptoPassConfirm->text())
			{
				message.setText(tr("Encryption password and confirmation do not match."));
				message.exec();
				return false;
			}
		}
	}

	return true;
}

void SetupWizard::handleFinished()
{
	close();

	AppConfig& appConfig = m_MainWindow.appConfig();

	appConfig.setCryptoMode(parseCryptoMode(m_pComboCryptoMode->currentText()));
	appConfig.setCryptoPass(m_pLineEditCryptoPass->text());

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
		m_MainWindow.start(true);
	}
}

void SetupWizard::on_m_pComboCryptoMode_currentIndexChanged(int index)
{
	bool enabled = parseCryptoMode(m_pComboCryptoMode->currentText()) != Disabled;
	m_pLineEditCryptoPass->setEnabled(enabled);
	m_pLineEditCryptoPassConfirm->setEnabled(enabled);
}

CryptoMode SetupWizard::parseCryptoMode(const QString& s)
{
	if (s.startsWith("OFB"))
	{
		return OFB;
	}
	else if (s.startsWith("CFB"))
	{
		return CFB;
	}
	else if (s.startsWith("CTR"))
	{
		return CTR;
	}
	else if (s.startsWith("GCM"))
	{
		return GCM;
	}

	return Disabled;
}
