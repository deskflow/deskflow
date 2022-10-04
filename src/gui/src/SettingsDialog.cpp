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
#include "validators/ScreenNameValidator.h"

#include "CoreInterface.h"
#include "SynergyLocale.h"
#include "QSynergyApplication.h"
#include "QUtility.h"
#include "AppConfig.h"
#include "SslCertificate.h"
#include "MainWindow.h"
#include "BonjourWindows.h"
#include "Zeroconf.h"
#include "UpgradeDialog.h"

#include <QtCore>
#include <QtGui>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>

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

    loadFromConfig();
    m_isSystemAtStart = appConfig().isSystemScoped();
    buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
    enableControls(appConfig().isWritable());

    m_pCheckBoxLanguageSync->setVisible(isClientMode());
    m_pCheckBoxScrollDirection->setVisible(isClientMode());

    const auto& serveConfig = m_pMainWindow->serverConfig();
    m_pLineEditScreenName->setValidator(new validators::ScreenNameValidator(m_pLineEditScreenName, m_pLabelNameError, (&serveConfig.screens())));

    connect(m_pLineEditLogFilename,     SIGNAL(textChanged(QString)),     this, SLOT(onChange()));
    connect(m_pComboLogLevel,           SIGNAL(currentIndexChanged(int)), this, SLOT(onChange()));
    connect(m_pLineEditCertificatePath, SIGNAL(textChanged(QString)),     this, SLOT(onChange()));
    connect(m_pCheckBoxAutoConfig,      SIGNAL(clicked()),                this, SLOT(onChange()));
    connect(m_pCheckBoxMinimizeToTray,  SIGNAL(clicked()),                this, SLOT(onChange()));
    connect(m_pCheckBoxAutoHide,        SIGNAL(clicked()),                this, SLOT(onChange()));
    connect(m_pCheckBoxPreventSleep,    SIGNAL(clicked()),                this, SLOT(onChange()));
    connect(m_pLineEditInterface,       SIGNAL(textEdited(QString)),      this, SLOT(onChange()));
    connect(m_pSpinBoxPort,             SIGNAL(valueChanged(int)),        this, SLOT(onChange()));
    connect(m_pLineEditScreenName,      SIGNAL(textEdited(QString)),      this, SLOT(onChange()));
    connect(m_pComboElevate,            SIGNAL(currentIndexChanged(int)), this, SLOT(onChange()));
    connect(m_pCheckBoxLanguageSync,    SIGNAL(clicked()),                this, SLOT(onChange()));
    connect(m_pCheckBoxScrollDirection, SIGNAL(clicked()),                this, SLOT(onChange()));
    connect(m_pCheckBoxClientHostMode,  SIGNAL(clicked()),                this, SLOT(onChange()));
    connect(m_pCheckBoxServerClientMode,SIGNAL(clicked()),                this, SLOT(onChange()));

    adjustSize();
}

void SettingsDialog::accept()
{
   appConfig().setLoadFromSystemScope(m_pRadioSystemScope->isChecked());
   appConfig().setScreenName(m_pLineEditScreenName->text());
   appConfig().setPort(m_pSpinBoxPort->value());
   appConfig().setNetworkInterface(m_pLineEditInterface->text());
   appConfig().setLogLevel(m_pComboLogLevel->currentIndex());
   appConfig().setLogToFile(m_pCheckBoxLogToFile->isChecked());
   appConfig().setLogFilename(m_pLineEditLogFilename->text());
   appConfig().setLanguage(m_pComboLanguage->itemData(m_pComboLanguage->currentIndex()).toString());
   appConfig().setElevateMode(static_cast<ElevateMode>(m_pComboElevate->currentIndex()));
   appConfig().setAutoHide(m_pCheckBoxAutoHide->isChecked());
   appConfig().setPreventSleep(m_pCheckBoxPreventSleep->isChecked());
   appConfig().setAutoConfig(m_pCheckBoxAutoConfig->isChecked());
   appConfig().setMinimizeToTray(m_pCheckBoxMinimizeToTray->isChecked());
   appConfig().setTLSCertPath(m_pLineEditCertificatePath->text());
   appConfig().setTLSKeyLength(m_pComboBoxKeyLength->currentText());
   appConfig().setCryptoEnabled(m_pCheckBoxEnableCrypto->isChecked());
   appConfig().setLanguageSync(m_pCheckBoxLanguageSync->isChecked());
   appConfig().setInvertScrollDirection(m_pCheckBoxScrollDirection->isChecked());
   appConfig().setClientHostMode(m_pCheckBoxClientHostMode->isChecked());
   appConfig().setServerClientMode(m_pCheckBoxServerClientMode->isChecked());

   appConfig().saveSettings();
   QDialog::accept();
}

void SettingsDialog::reject()
{
    if (appConfig().language() != m_pComboLanguage->itemData(m_pComboLanguage->currentIndex()).toString()) {
        QSynergyApplication::getInstance()->switchTranslator(appConfig().language());
    }

    // We should restore scope at start if the user rejects changes.
    if (appConfig().isSystemScoped() != m_isSystemAtStart) {
        appConfig().setLoadFromSystemScope(m_isSystemAtStart);
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

void SettingsDialog::loadFromConfig() {

    m_pLineEditScreenName->setText(appConfig().screenName());
    m_pSpinBoxPort->setValue(appConfig().port());
    m_pLineEditInterface->setText(appConfig().networkInterface());
    m_pComboLogLevel->setCurrentIndex(appConfig().logLevel());
    m_pCheckBoxLogToFile->setChecked(appConfig().logToFile());
    m_pLineEditLogFilename->setText(appConfig().logFilename());
    setIndexFromItemData(m_pComboLanguage, appConfig().language());
    m_pCheckBoxAutoHide->setChecked(appConfig().getAutoHide());
    m_pCheckBoxPreventSleep->setChecked(appConfig().getPreventSleep());
    m_pCheckBoxMinimizeToTray->setChecked(appConfig().getMinimizeToTray());
    m_pLineEditCertificatePath->setText(appConfig().getTLSCertPath());
    m_pCheckBoxEnableCrypto->setChecked(m_appConfig.getCryptoEnabled());
    m_pCheckBoxLanguageSync->setChecked(m_appConfig.getLanguageSync());
    m_pCheckBoxScrollDirection->setChecked(m_appConfig.getInvertScrollDirection());
    m_pCheckBoxClientHostMode->setChecked(m_appConfig.getClientHostMode());
    m_pCheckBoxServerClientMode->setChecked(m_appConfig.getServerClientMode());

    setupSeurity();

    if (m_appConfig.isSystemScoped()) {
        m_pRadioSystemScope->setChecked(true);
    }
    else {
        m_pRadioUserScope->setChecked(true);
    }

#if defined(Q_OS_WIN)
    m_pBonjourWindows = new BonjourWindows(this, m_pMainWindow, m_appConfig);
    if (m_pBonjourWindows->isRunning()) {
        allowAutoConfig();
    }

    m_pComboElevate->setCurrentIndex(static_cast<int>(appConfig().elevateMode()));

#else
    // elevate checkbox is only useful on ms windows.
    m_pLabelElevate->hide();
    m_pComboElevate->hide();

    // for linux and mac, allow auto config by default
    allowAutoConfig();
#endif

#if !defined(SYNERGY_ENTERPRISE) && defined(SYNERGY_AUTOCONFIG)
    m_pCheckBoxAutoConfig->setChecked(appConfig().autoConfig());
#else
    m_pCheckBoxAutoConfig->hide();
    m_pLabelInstallBonjour->hide();
#endif

    m_pCheckBoxClientHostMode->setVisible(isClientMode() && appConfig().getInitiateConnectionFromServer());
    m_pCheckBoxServerClientMode->setVisible(!isClientMode() && appConfig().getInitiateConnectionFromServer());
}

void SettingsDialog::setupSeurity()
{
    //If the tls file exists test its key length
    if (QFile(appConfig().getTLSCertPath()).exists()) {
        updateKeyLengthOnFile(appConfig().getTLSCertPath());
    } else {
        m_pComboBoxKeyLength->setCurrentIndex(m_pComboBoxKeyLength->findText(appConfig().getTLSKeyLength()));
    }

    m_pCheckBoxEnableCrypto->setChecked(m_appConfig.getCryptoEnabled());

    if (appConfig().getClientGroupChecked()) {
        m_pLabelKeyLength->hide();
        m_pComboBoxKeyLength->hide();
        m_pLabelCertificate->hide();
        m_pLineEditCertificatePath->hide();
        m_pPushButtonBrowseCert->hide();
        m_pPushButtonRegenCert->hide();
    }
}

bool SettingsDialog::isClientMode() const
{
    return (m_pMainWindow->synergyType() == MainWindow::synergyClient);
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

    m_pLabelLogPath->setEnabled(checked);
    m_pLineEditLogFilename->setEnabled(checked);
    m_pButtonBrowseLog->setEnabled(checked);
    buttonBox->button(QDialogButtonBox::Save)->setEnabled(isModified());
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
    buttonBox->button(QDialogButtonBox::Save)->setEnabled(isModified());
}

void SettingsDialog::on_m_pCheckBoxEnableCrypto_clicked(bool checked)
{
    if (appConfig().isCryptoAvailable())
    {
        m_pLabelKeyLength->setEnabled(checked);
        m_pComboBoxKeyLength->setEnabled(checked);
        m_pLabelCertificate->setEnabled(checked);
        m_pLineEditCertificatePath->setEnabled(checked);
        m_pPushButtonBrowseCert->setEnabled(checked);
        m_pPushButtonRegenCert->setEnabled(checked);

        buttonBox->button(QDialogButtonBox::Save)->setEnabled(isModified());
    }
    else
    {
        m_pCheckBoxEnableCrypto->setChecked(false);
#if !defined(SYNERGY_ENTERPRISE) && !defined(SYNERGY_BUSINESS)
        UpgradeDialog upgradeDialog(this);
        if (appConfig().edition() == Edition::kLite)
        {
            upgradeDialog.showDialog(
                "TLS encryption is a Synergy Ultimate feature.",
                "synergy/purchase/purchase-ultimate-upgrade?source=gui"
            );
        }
        else
        {
            upgradeDialog.showDialog(
                "TLS encryption is a Synergy Pro feature.",
                "synergy/purchase/upgrade?source=gui"
            );
        }
#endif
    }
}

void SettingsDialog::on_m_pLabelInstallBonjour_linkActivated(const QString&)
{
#if defined(Q_OS_WIN)
    m_pBonjourWindows->downloadAndInstall();
#endif
}

void SettingsDialog::on_m_pRadioSystemScope_toggled(bool checked)
{
    //We only need to test the System scoped Radio as they are connected
    appConfig().setLoadFromSystemScope(checked);
    loadFromConfig();
    buttonBox->button(QDialogButtonBox::Save)->setEnabled(m_isSystemAtStart != checked);
    enableControls(appConfig().isWritable());
}

void SettingsDialog::on_m_pPushButtonBrowseCert_clicked() {
    QString fileName = QFileDialog::getSaveFileName(
            this, tr("Select a TLS certificate to use..."),
            m_pLineEditCertificatePath->text(),
            "Cert (*.pem)",
            nullptr,
            QFileDialog::DontConfirmOverwrite);

    if (!fileName.isEmpty()) {
        m_pLineEditCertificatePath->setText(fileName);
        //If the tls file exists test its key length and update
        if (QFile(appConfig().getTLSCertPath()).exists()) {
            updateKeyLengthOnFile(fileName);
        }
    }
    updateRegenButton();
}

void SettingsDialog::on_m_pComboBoxKeyLength_currentIndexChanged(int index) {
    buttonBox->button(QDialogButtonBox::Save)->setEnabled(isModified());
    updateRegenButton();
}

void SettingsDialog::updateRegenButton() {
    // Disable the Regenerate cert button if the key length is different to saved
    auto keyChanged = appConfig().getTLSKeyLength() != m_pComboBoxKeyLength->currentText();
    auto pathChanged = appConfig().getTLSCertPath() != m_pLineEditCertificatePath->text();
    //NOR the above bools, if any have changed regen should be disabled as it will be done on save
    auto nor = !(keyChanged || pathChanged);
    m_pPushButtonRegenCert->setEnabled(nor && m_pCheckBoxEnableCrypto->isChecked());
}

void SettingsDialog::on_m_pPushButtonRegenCert_clicked() {
    appConfig().generateCertificate(true);
}

void SettingsDialog::updateKeyLengthOnFile(const QString &path) {
    SslCertificate ssl;
    auto length = ssl.getCertKeyLength(path);
    auto index = m_pComboBoxKeyLength->findText(length);
    m_pComboBoxKeyLength->setCurrentIndex(index);
    //Also update what is in the appconfig to match the file itself
    appConfig().setTLSKeyLength(length);
}

bool SettingsDialog::isModified()
{
   return (!m_pLineEditScreenName->text().isEmpty() &&
      m_pLabelNameError->text().isEmpty() &&
      (appConfig().screenName()          != m_pLineEditScreenName->text()
      || appConfig().port()              != m_pSpinBoxPort->value()
      || appConfig().networkInterface()  != m_pLineEditInterface->text()
      || appConfig().logLevel()          != m_pComboLogLevel->currentIndex()
      || appConfig().logToFile()         != m_pCheckBoxLogToFile->isChecked()
      || appConfig().logFilename()       != m_pLineEditLogFilename->text()
      || appConfig().language()          != m_pComboLanguage->itemData(m_pComboLanguage->currentIndex()).toString()
      || appConfig().elevateMode()       != static_cast<ElevateMode>(m_pComboElevate->currentIndex())
      || appConfig().getAutoHide()       != m_pCheckBoxAutoHide->isChecked()
      || appConfig().getPreventSleep()   != m_pCheckBoxPreventSleep->isChecked()
      || appConfig().autoConfig()        != m_pCheckBoxAutoConfig->isChecked()
      || appConfig().getMinimizeToTray() != m_pCheckBoxMinimizeToTray->isChecked()
      || appConfig().getTLSCertPath()    != m_pLineEditCertificatePath->text()
      || appConfig().getTLSKeyLength()   != m_pComboBoxKeyLength->currentText()
      || appConfig().getCryptoEnabled()  != m_pCheckBoxEnableCrypto->isChecked()
      || appConfig().isSystemScoped()    != m_isSystemAtStart
      || appConfig().getLanguageSync()   != m_pCheckBoxLanguageSync->isChecked()
      || appConfig().getClientHostMode() != m_pCheckBoxClientHostMode->isChecked()
      || appConfig().getServerClientMode() != m_pCheckBoxServerClientMode->isChecked()
      || appConfig().getInvertScrollDirection() != m_pCheckBoxScrollDirection->isChecked())
   );
}

void SettingsDialog::enableControls(bool enable) {
    m_pLineEditScreenName->setEnabled(enable);
    m_pSpinBoxPort->setEnabled(enable);
    m_pLineEditInterface->setEnabled(enable);
    m_pComboLogLevel->setEnabled(enable);
    m_pCheckBoxLogToFile->setEnabled(enable);
    m_pComboLanguage->setEnabled(enable);
    m_pComboElevate->setEnabled(enable);
    m_pCheckBoxAutoHide->setEnabled(enable);
    m_pCheckBoxPreventSleep->setEnabled(enable);
    m_pCheckBoxAutoConfig->setEnabled(enable);
    m_pCheckBoxMinimizeToTray->setEnabled(enable);
    m_pLineEditCertificatePath->setEnabled(enable);
    m_pComboBoxKeyLength->setEnabled(enable);
    m_pPushButtonBrowseCert->setEnabled(enable);
    m_pCheckBoxEnableCrypto->setEnabled(enable);
    m_labelAdminRightsMessage->setVisible(!enable);
    m_pCheckBoxLanguageSync->setEnabled(enable);
    m_pCheckBoxScrollDirection->setEnabled(enable);
    m_pCheckBoxClientHostMode->setEnabled(enable);
    m_pCheckBoxServerClientMode->setEnabled(enable);

    if (enable) {
        m_pLabelLogPath->setEnabled(m_pCheckBoxLogToFile->isChecked());
        m_pLineEditLogFilename->setEnabled(m_pCheckBoxLogToFile->isChecked());
        m_pButtonBrowseLog->setEnabled(m_pCheckBoxLogToFile->isChecked());
        m_pLabelKeyLength->setEnabled(m_pCheckBoxEnableCrypto->isChecked());
        m_pComboBoxKeyLength->setEnabled(m_pCheckBoxEnableCrypto->isChecked());
        m_pLabelCertificate->setEnabled(m_pCheckBoxEnableCrypto->isChecked());
        m_pLineEditCertificatePath->setEnabled(m_pCheckBoxEnableCrypto->isChecked());
        m_pPushButtonBrowseCert->setEnabled(m_pCheckBoxEnableCrypto->isChecked());
        updateRegenButton();
    }
    else {
        m_pLabelLogPath->setEnabled(enable);
        m_pLineEditLogFilename->setEnabled(enable);
        m_pButtonBrowseLog->setEnabled(enable);
        m_pLabelKeyLength->setEnabled(enable);
        m_pComboBoxKeyLength->setEnabled(enable);
        m_pLabelCertificate->setEnabled(enable);
        m_pLineEditCertificatePath->setEnabled(enable);
        m_pPushButtonBrowseCert->setEnabled(enable);
        m_pPushButtonRegenCert->setEnabled(enable);
    }
}

void SettingsDialog::onChange()
{
   buttonBox->button(QDialogButtonBox::Save)->setEnabled(isModified());
}
