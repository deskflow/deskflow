/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2008 Volker Lanz (vl@fidra.de)
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

#include "SettingsDialog.h"

#include "CoreInterface.h"
#include "SynergyLocale.h"
#include "QSynergyApplication.h"
#include "QUtility.h"
#include "AppConfig.h"
#include "SslCertificate.h"
#include "MainWindow.h"
#include "BonjourWindows.h"
#include "Zeroconf.h"

#include <QtCore>
#include <QtGui>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>

static const char networkSecurity[] = "ns";

SettingsDialog::SettingsDialog(QWidget* parent, AppConfig& config) :
    QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
    Ui::SettingsDialogBase(),
    m_appConfig(config),
    m_pBonjourWindows(nullptr)
{
    setupUi(this);

    // TODO: maybe just accept MainWindow type in ctor?
    m_pMainWindow = dynamic_cast<MainWindow*>(parent);

    m_Locale.fillLanguageComboBox(m_pComboLanguage);

    m_pLineEditScreenName->setText(appConfig().screenName());
    m_pSpinBoxPort->setValue(appConfig().port());
    m_pLineEditInterface->setText(appConfig().networkInterface());
    m_pComboLogLevel->setCurrentIndex(appConfig().logLevel());
    m_pCheckBoxLogToFile->setChecked(appConfig().logToFile());
    m_pLineEditLogFilename->setText(appConfig().logFilename());
    setIndexFromItemData(m_pComboLanguage, appConfig().language());
    m_pCheckBoxAutoHide->setChecked(appConfig().getAutoHide());

#if defined(Q_OS_WIN)
    m_pBonjourWindows = new BonjourWindows(this, m_pMainWindow, m_appConfig);
    if (m_pBonjourWindows->isRunning()) {
        allowAutoConfig();
    }

    m_pComboElevate->setCurrentIndex(static_cast<int>(appConfig().elevateMode()));

    m_pCheckBoxAutoHide->hide();
#else
    // elevate checkbox is only useful on ms windows.
    m_pLabelElevate->hide();
    m_pComboElevate->hide();

    // for linux and mac, allow auto config by default
    allowAutoConfig();
#endif

    m_pCheckBoxEnableCrypto->setChecked(m_appConfig.getCryptoEnabled());

#ifdef SYNERGY_ENTERPRISE

     m_pCheckBoxEnableCrypto->setEnabled(true);
     m_pLabelProUpgrade->hide();

     m_pCheckBoxAutoConfig->hide();
     m_pLabelInstallBonjour->hide();

#else

    bool isPro = m_appConfig.edition() == kPro;
    m_pCheckBoxEnableCrypto->setEnabled(isPro);
    m_pLabelProUpgrade->setVisible(!isPro);

    m_pCheckBoxAutoConfig->setChecked(appConfig().autoConfig());

#endif
}

void SettingsDialog::accept()
{
    appConfig().setScreenName(m_pLineEditScreenName->text());
    appConfig().setPort(m_pSpinBoxPort->value());
    appConfig().setNetworkInterface(m_pLineEditInterface->text());
    appConfig().setLogLevel(m_pComboLogLevel->currentIndex());
    appConfig().setLogToFile(m_pCheckBoxLogToFile->isChecked());
    appConfig().setLogFilename(m_pLineEditLogFilename->text());
    appConfig().setLanguage(m_pComboLanguage->itemData(m_pComboLanguage->currentIndex()).toString());
       appConfig().setElevateMode(static_cast<ElevateMode>(m_pComboElevate->currentIndex()));
    appConfig().setAutoHide(m_pCheckBoxAutoHide->isChecked());
    appConfig().setAutoConfig(m_pCheckBoxAutoConfig->isChecked());
    appConfig().saveSettings();
    QDialog::accept();
}

void SettingsDialog::reject()
{
    if (appConfig().language() != m_pComboLanguage->itemData(m_pComboLanguage->currentIndex()).toString()) {
        QSynergyApplication::getInstance()->switchTranslator(appConfig().language());
    }

    QDialog::reject();
}

void SettingsDialog::changeEvent(QEvent* event)
{
    if (event != nullptr)
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

void SettingsDialog::allowAutoConfig()
{
    m_pLabelInstallBonjour->hide();
    m_pCheckBoxAutoConfig->setEnabled(true);
    m_pCheckBoxAutoConfig->setChecked(m_appConfig.autoConfig());
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

void SettingsDialog::on_m_pComboLanguage_currentIndexChanged(int index)
{
    QString ietfCode = m_pComboLanguage->itemData(index).toString();
    QSynergyApplication::getInstance()->switchTranslator(ietfCode);
}

void SettingsDialog::on_m_pCheckBoxEnableCrypto_toggled(bool checked)
{
    m_appConfig.setCryptoEnabled(checked);
    m_appConfig.saveSettings();
    if (checked) {
        SslCertificate sslCertificate;
        sslCertificate.generateCertificate();
        m_pMainWindow->updateLocalFingerprint();
    }
}

void SettingsDialog::on_m_pLabelInstallBonjour_linkActivated(const QString&)
{
#if defined(Q_OS_WIN)
    m_pBonjourWindows->downloadAndInstall();
#endif
}
