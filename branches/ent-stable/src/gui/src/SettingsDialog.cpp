/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
 * Copyright (C) 2008 Volker Lanz (vl@fidra.de)
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

#include "SettingsDialog.h"
#include "SynergyLocale.h"
#include "QSynergyApplication.h"
#include "QUtility.h"
#include "AppConfig.h"

#include <QtCore>
#include <QtGui>
#include <QMessageBox>
#include <QFileDialog>

SettingsDialog::SettingsDialog(QWidget* parent, AppConfig& config) :
	QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	Ui::SettingsDialogBase(),
	m_AppConfig(config)
{
	setupUi(this);

	m_Locale.fillLanguageComboBox(m_pComboLanguage);

	m_pLineEditScreenName->setText(appConfig().screenName());
	m_pSpinBoxPort->setValue(appConfig().port());
	m_pLineEditInterface->setText(appConfig().interface());
	m_pComboLogLevel->setCurrentIndex(appConfig().logLevel());
	m_pCheckBoxLogToFile->setChecked(appConfig().logToFile());
	m_pLineEditLogFilename->setText(appConfig().logFilename());
	m_pCheckBoxEnableCrypto->setChecked(appConfig().cryptoEnabled());
	setIndexFromItemData(m_pComboLanguage, appConfig().language());
	if (appConfig().cryptoEnabled())
	{
		m_pLineEditCryptoPass->setText(appConfig().cryptoPass());
	}
}

void SettingsDialog::accept()
{
	const QString& cryptoPass = m_pLineEditCryptoPass->text();
	bool cryptoEnabled = m_pCheckBoxEnableCrypto->isChecked();
	if (cryptoEnabled && cryptoPass.isEmpty())
	{
		QMessageBox message;
		message.setWindowTitle("Settings");
		message.setIcon(QMessageBox::Information);
		message.setText(tr("Encryption password must not be empty."));
		message.exec();
		return;
	}

	appConfig().setScreenName(m_pLineEditScreenName->text());
	appConfig().setPort(m_pSpinBoxPort->value());
	appConfig().setInterface(m_pLineEditInterface->text());
	appConfig().setLogLevel(m_pComboLogLevel->currentIndex());
	appConfig().setLogToFile(m_pCheckBoxLogToFile->isChecked());
	appConfig().setLogFilename(m_pLineEditLogFilename->text());
	appConfig().setCryptoEnabled(cryptoEnabled);
	appConfig().setCryptoPass(cryptoPass);
	appConfig().setLanguage(m_pComboLanguage->itemData(m_pComboLanguage->currentIndex()).toString());
	appConfig().saveSettings();
	QDialog::accept();
}

void SettingsDialog::reject()
{
	QSynergyApplication::getInstance()->switchTranslator(appConfig().language());
	QDialog::reject();
}

void SettingsDialog::changeEvent(QEvent* event)
{
	if (event != 0)
	{
		switch (event->type())
		{
		case QEvent::LanguageChange:
			{
				int logLevelIndex = m_pComboLogLevel->currentIndex();

				m_pComboLanguage->blockSignals(true);
				retranslateUi(this);
				m_pComboLanguage->blockSignals(false);

				m_pComboLogLevel->setCurrentIndex(logLevelIndex);
				break;
			}

		default:
			QDialog::changeEvent(event);
		}
	}
}

void SettingsDialog::on_m_pCheckBoxLogToFile_stateChanged(int i)
{
	bool checked = i == 2;

	m_pLineEditLogFilename->setEnabled(checked);
	m_pButtonBrowseLog->setEnabled(checked);
}

void SettingsDialog::on_m_pButtonBrowseLog_clicked()
{
	QString fileName = QFileDialog::getSaveFileName(
		this, tr("Save log file to..."),
		m_pLineEditLogFilename->text(),
		"Logs (*.log *.txt)");

	if (!fileName.isEmpty())
	{
		m_pLineEditLogFilename->setText(fileName);
	}
}

void SettingsDialog::on_m_pCheckBoxEnableCrypto_stateChanged(int )
{
	bool cryptoEnabled = m_pCheckBoxEnableCrypto->isChecked();
	m_pLineEditCryptoPass->setEnabled(cryptoEnabled);

	if (!cryptoEnabled)
	{
		m_pLineEditCryptoPass->clear();
	}
}

void SettingsDialog::on_m_pComboLanguage_currentIndexChanged(int index)
{
	QString ietfCode = m_pComboLanguage->itemData(index).toString();
	QSynergyApplication::getInstance()->switchTranslator(ietfCode);
}
