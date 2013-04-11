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

#include <QtCore>
#include <QtGui>
#include <QCryptographicHash>

#include "AppConfig.h"

SettingsDialog::SettingsDialog(QWidget* parent, AppConfig& config) :
	QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	Ui::SettingsDialogBase(),
	m_AppConfig(config)
{
	setupUi(this);

	m_pCheckBoxAutoConnect->setChecked(appConfig().autoConnect());
	m_pLineEditScreenName->setText(appConfig().screenName());
	m_pSpinBoxPort->setValue(appConfig().port());
	m_pLineEditInterface->setText(appConfig().interface());
	m_pComboProcessMode->setCurrentIndex(appConfig().processMode());
	m_pComboLogLevel->setCurrentIndex(appConfig().logLevel());
	m_pCheckBoxLogToFile->setChecked(appConfig().logToFile());
	m_pLineEditLogFilename->setText(appConfig().logFilename());
	m_pCheckBoxAutoStart->setChecked(appConfig().autoStart());
	m_pCheckBoxAutoHide->setChecked(appConfig().autoHide());
	m_pComboCryptoMode->setCurrentIndex(getCryptoModeIndex(appConfig().cryptoMode()));
	m_pLineEditCryptoPass->setText(appConfig().cryptoPass());
}

void SettingsDialog::accept()
{
	const QString& cryptoPass = m_pLineEditCryptoPass->text();
	CryptoMode cryptoMode = parseCryptoMode(m_pComboCryptoMode->currentText());
	if ((cryptoMode != Disabled) && cryptoPass.isEmpty())
	{
		QMessageBox message;
		message.setWindowTitle("Settings");
		message.setIcon(QMessageBox::Information);
		message.setText(tr("Encryption password must not be empty."));
		message.exec();
		return;
	}

	appConfig().setAutoConnect(m_pCheckBoxAutoConnect->isChecked());
	appConfig().setScreenName(m_pLineEditScreenName->text());
	appConfig().setPort(m_pSpinBoxPort->value());
	appConfig().setInterface(m_pLineEditInterface->text());
	appConfig().setProcessMode((ProcessMode)m_pComboProcessMode->currentIndex());
	appConfig().setLogLevel(m_pComboLogLevel->currentIndex());
	appConfig().setLogToFile(m_pCheckBoxLogToFile->isChecked());
	appConfig().setLogFilename(m_pLineEditLogFilename->text());
	appConfig().setAutoStart(m_pCheckBoxAutoStart->isChecked());
	appConfig().setAutoHide(m_pCheckBoxAutoHide->isChecked());
	appConfig().setCryptoMode(cryptoMode);
	appConfig().setCryptoPass(cryptoPass);
	appConfig().saveSettings();
	QDialog::accept();
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

void SettingsDialog::on_m_pComboCryptoMode_currentIndexChanged(int index)
{
	bool enabled = parseCryptoMode(m_pComboCryptoMode->currentText()) != Disabled;
	m_pLineEditCryptoPass->setEnabled(enabled);
	if (!enabled)
	{
		m_pLineEditCryptoPass->clear();
	}
}

int SettingsDialog::getCryptoModeIndex(const CryptoMode& mode) const
{
	switch (mode)
	{
	case OFB:
		return m_pComboCryptoMode->findText("OFB", Qt::MatchStartsWith);

	case CFB:
		return m_pComboCryptoMode->findText("CFB", Qt::MatchStartsWith);

	case CTR:
		return m_pComboCryptoMode->findText("CTR", Qt::MatchStartsWith);

	case GCM:
		return m_pComboCryptoMode->findText("GCM", Qt::MatchStartsWith);

	default:
		return m_pComboCryptoMode->findText("Disable", Qt::MatchStartsWith);
	}
}

CryptoMode SettingsDialog::parseCryptoMode(const QString& s)
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
