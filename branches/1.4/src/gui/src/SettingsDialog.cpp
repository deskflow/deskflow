/*
 * synergy -- mouse and keyboard sharing utility
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
	m_pComboLogLevel->setCurrentIndex(appConfig().logLevel());
	m_pCheckBoxLogToFile->setChecked(appConfig().logToFile());
	m_pLineEditLogFilename->setText(appConfig().logFilename());
	m_pCheckBoxAutoStart->setChecked(appConfig().autoStart());
	m_pCheckBoxAutoHide->setChecked(appConfig().autoHide());

#ifdef Q_OS_WIN
	m_pCheckBoxGameDevice->setChecked(appConfig().gameDevice());
#else
	m_pCheckBoxGameDevice->setEnabled(false);
#endif
}

void SettingsDialog::accept()
{
	appConfig().setAutoConnect(m_pCheckBoxAutoConnect->isChecked());
	appConfig().setScreenName(m_pLineEditScreenName->text());
	appConfig().setPort(m_pSpinBoxPort->value());
	appConfig().setInterface(m_pLineEditInterface->text());
	appConfig().setLogLevel(m_pComboLogLevel->currentIndex());
	appConfig().setLogToFile(m_pCheckBoxLogToFile->isChecked());
	appConfig().setLogFilename(m_pLineEditLogFilename->text());
	appConfig().setGameDevice(m_pCheckBoxGameDevice->isChecked());
	appConfig().setAutoStart(m_pCheckBoxAutoStart->isChecked());
	appConfig().setAutoHide(m_pCheckBoxAutoHide->isChecked());

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
