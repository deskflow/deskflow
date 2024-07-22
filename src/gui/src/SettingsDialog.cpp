/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Symless Ltd.
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

#include "AppConfig.h"
#include "MainWindow.h"
#include "SslCertificate.h"
#include "UpgradeDialog.h"

#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QtCore>
#include <QtGui>
#include <memory>

SettingsDialog::SettingsDialog(QWidget *parent, AppConfig &config)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
      Ui::SettingsDialogBase(),
      m_appConfig(config) {
  setupUi(this);

  // TODO: maybe just accept MainWindow type in ctor?
  m_pMainWindow = dynamic_cast<MainWindow *>(parent);

  loadFromConfig();
  m_isSystemAtStart = appConfig().isSystemScoped();
  enableControls(appConfig().isWritable());

  const auto &serveConfig = m_pMainWindow->serverConfig();
  m_screenNameValidator = std::make_unique<validators::ScreenNameValidator>(
      m_pLineEditScreenName, nullptr, (&serveConfig.screens()));
  connect(
      m_screenNameValidator.get(), SIGNAL(finished(QString)), this,
      SLOT(on_m_pScreenNameValidator_finished(QString)));
  m_pLineEditScreenName->setValidator(m_screenNameValidator.get());

  connect(
      m_pLineEditLogFilename, SIGNAL(textChanged(QString)), this,
      SLOT(onChange()));
  connect(
      m_pComboLogLevel, SIGNAL(currentIndexChanged(int)), this,
      SLOT(onChange()));
  connect(
      m_pLineEditCertificatePath, SIGNAL(textChanged(QString)), this,
      SLOT(onChange()));
  connect(m_pCheckBoxMinimizeToTray, SIGNAL(clicked()), this, SLOT(onChange()));
  connect(m_pCheckBoxAutoHide, SIGNAL(clicked()), this, SLOT(onChange()));
  connect(m_pCheckBoxPreventSleep, SIGNAL(clicked()), this, SLOT(onChange()));
  connect(
      m_pLineEditInterface, SIGNAL(textEdited(QString)), this,
      SLOT(onChange()));
  connect(m_pSpinBoxPort, SIGNAL(valueChanged(int)), this, SLOT(onChange()));
  connect(
      m_pLineEditScreenName, SIGNAL(textEdited(QString)), this,
      SLOT(onChange()));
  connect(
      m_pComboElevate, SIGNAL(currentIndexChanged(int)), this,
      SLOT(onChange()));
  connect(m_pCheckBoxLanguageSync, SIGNAL(clicked()), this, SLOT(onChange()));
  connect(
      m_pCheckBoxScrollDirection, SIGNAL(clicked()), this, SLOT(onChange()));
  connect(m_pCheckBoxClientHostMode, SIGNAL(clicked()), this, SLOT(onChange()));
  connect(
      m_pCheckBoxServerClientMode, SIGNAL(clicked()), this, SLOT(onChange()));

  adjustSize();
}

void SettingsDialog::accept() {
  if (!m_nameError.isEmpty()) {
    QMessageBox::warning(this, tr("Invalid screen name"), m_nameError);
    return;
  }

  appConfig().setLoadFromSystemScope(m_pRadioSystemScope->isChecked());
  appConfig().setScreenName(m_pLineEditScreenName->text());
  appConfig().setPort(m_pSpinBoxPort->value());
  appConfig().setNetworkInterface(m_pLineEditInterface->text());
  appConfig().setLogLevel(m_pComboLogLevel->currentIndex());
  appConfig().setLogToFile(m_pCheckBoxLogToFile->isChecked());
  appConfig().setLogFilename(m_pLineEditLogFilename->text());
  appConfig().setElevateMode(
      static_cast<ElevateMode>(m_pComboElevate->currentIndex()));
  appConfig().setAutoHide(m_pCheckBoxAutoHide->isChecked());
  appConfig().setPreventSleep(m_pCheckBoxPreventSleep->isChecked());
  appConfig().setMinimizeToTray(m_pCheckBoxMinimizeToTray->isChecked());
  appConfig().setTlsCertPath(m_pLineEditCertificatePath->text());
  appConfig().setTlsKeyLength(m_pComboBoxKeyLength->currentText());
  appConfig().setCryptoEnabled(m_pCheckBoxEnableCrypto->isChecked());
  appConfig().setLanguageSync(m_pCheckBoxLanguageSync->isChecked());
  appConfig().setInvertScrollDirection(m_pCheckBoxScrollDirection->isChecked());
  appConfig().setClientHostMode(m_pCheckBoxClientHostMode->isChecked());
  appConfig().setServerClientMode(m_pCheckBoxServerClientMode->isChecked());
  appConfig().setServiceEnabled(m_pCheckBoxServiceEnabled->isChecked());
  appConfig().setMinimizeOnClose(m_pCheckBoxMinimizeOnClose->isChecked());

  appConfig().saveSettings();
  QDialog::accept();
}

void SettingsDialog::reject() {
  // We should restore scope at start if the user rejects changes.
  if (appConfig().isSystemScoped() != m_isSystemAtStart) {
    appConfig().setLoadFromSystemScope(m_isSystemAtStart);
  }

  QDialog::reject();
}

void SettingsDialog::loadFromConfig() {

  m_pLineEditScreenName->setText(appConfig().screenName());
  m_pSpinBoxPort->setValue(appConfig().port());
  m_pLineEditInterface->setText(appConfig().networkInterface());
  m_pComboLogLevel->setCurrentIndex(appConfig().logLevel());
  m_pCheckBoxLogToFile->setChecked(appConfig().logToFile());
  m_pLineEditLogFilename->setText(appConfig().logFilename());
  m_pCheckBoxAutoHide->setChecked(appConfig().autoHide());
  m_pCheckBoxPreventSleep->setChecked(appConfig().preventSleep());
  m_pCheckBoxMinimizeToTray->setChecked(appConfig().minimizeToTray());
  m_pLineEditCertificatePath->setText(appConfig().tlsCertPath());
  m_pCheckBoxEnableCrypto->setChecked(m_appConfig.cryptoEnabled());
  m_pCheckBoxLanguageSync->setChecked(m_appConfig.languageSync());
  m_pCheckBoxScrollDirection->setChecked(m_appConfig.invertScrollDirection());
  m_pCheckBoxClientHostMode->setChecked(m_appConfig.clientHostMode());
  m_pCheckBoxServerClientMode->setChecked(m_appConfig.serverClientMode());
  m_pCheckBoxServiceEnabled->setChecked(m_appConfig.serviceEnabled());
  m_pCheckBoxMinimizeOnClose->setChecked(m_appConfig.minimizeOnClose());

  if (m_appConfig.isSystemScoped()) {
    m_pRadioSystemScope->setChecked(true);
  } else {
    m_pRadioUserScope->setChecked(true);
  }

  setupSeurity();
}

void SettingsDialog::setupSeurity() {
  // If the tls file exists test its key length
  if (QFile(appConfig().tlsCertPath()).exists()) {
    updateKeyLengthOnFile(appConfig().tlsCertPath());
  } else {
    m_pComboBoxKeyLength->setCurrentIndex(
        m_pComboBoxKeyLength->findText(appConfig().tlsKeyLength()));
  }

  m_pCheckBoxEnableCrypto->setChecked(m_appConfig.cryptoEnabled());

  if (appConfig().clientGroupChecked()) {
    m_pLabelKeyLength->setEnabled(false);
    m_pComboBoxKeyLength->setEnabled(false);
    m_pLabelCertificate->setEnabled(false);
    m_pLineEditCertificatePath->setEnabled(false);
    m_pPushButtonBrowseCert->setEnabled(false);
    m_pPushButtonRegenCert->setEnabled(false);
  }
}

bool SettingsDialog::isClientMode() const {
  return (m_pMainWindow->coreMode() == MainWindow::CoreMode::Client);
}

void SettingsDialog::on_m_pCheckBoxLogToFile_stateChanged(int i) {
  bool checked = i == 2;

  m_pLabelLogPath->setEnabled(checked);
  m_pLineEditLogFilename->setEnabled(checked);
  m_pButtonBrowseLog->setEnabled(checked);
}

void SettingsDialog::on_m_pButtonBrowseLog_clicked() {
  QString fileName = QFileDialog::getSaveFileName(
      this, tr("Save log file to..."), m_pLineEditLogFilename->text(),
      "Logs (*.log *.txt)");

  if (!fileName.isEmpty()) {
    m_pLineEditLogFilename->setText(fileName);
  }
}

void SettingsDialog::on_m_pCheckBoxEnableCrypto_clicked(bool checked) {
  if (appConfig().cryptoAvailable()) {
    m_pLabelKeyLength->setEnabled(checked);
    m_pComboBoxKeyLength->setEnabled(checked);
    m_pLabelCertificate->setEnabled(checked);
    m_pLineEditCertificatePath->setEnabled(checked);
    m_pPushButtonBrowseCert->setEnabled(checked);
    m_pPushButtonRegenCert->setEnabled(checked);
  } else {
    m_pCheckBoxEnableCrypto->setChecked(false);

#ifdef SYNERGY_ENABLE_LICENSING
    auto edition = appConfig().edition();
    if (edition == Edition::kLite || edition == Edition::kBasic) {
      UpgradeDialog upgradeDialog(this);
      if (appConfig().edition() == Edition::kLite) {
        upgradeDialog.showDialog(
            "Upgrade to Synergy Ultimate to enable TLS encryption.");
      } else if (appConfig().edition() == Edition::kBasic) {
        upgradeDialog.showDialog(
            "Upgrade to Synergy Pro to enable TLS encryption.");
      }
    }
#endif // SYNERGY_ENABLE_LICENSING
  }
}

void SettingsDialog::on_m_pRadioSystemScope_toggled(bool checked) {
  // We only need to test the System scoped Radio as they are connected
  appConfig().setLoadFromSystemScope(checked);
  loadFromConfig();
  enableControls(appConfig().isWritable());
}

void SettingsDialog::on_m_pPushButtonBrowseCert_clicked() {
  QString fileName = QFileDialog::getSaveFileName(
      this, tr("Select a TLS certificate to use..."),
      m_pLineEditCertificatePath->text(), "Cert (*.pem)", nullptr,
      QFileDialog::DontConfirmOverwrite);

  if (!fileName.isEmpty()) {
    m_pLineEditCertificatePath->setText(fileName);
    // If the tls file exists test its key length and update
    if (QFile(appConfig().tlsCertPath()).exists()) {
      updateKeyLengthOnFile(fileName);
    }
  }
  updateTlsRegenerateButton();
}

void SettingsDialog::on_m_pComboBoxKeyLength_currentIndexChanged(int index) {
  updateTlsRegenerateButton();
}

void SettingsDialog::updateTlsRegenerateButton() {
  // Disable the Regenerate cert button if the key length is different to saved
  auto keyChanged =
      appConfig().tlsKeyLength() != m_pComboBoxKeyLength->currentText();
  auto pathChanged =
      appConfig().tlsCertPath() != m_pLineEditCertificatePath->text();
  // NOR the above bools, if any have changed regen should be disabled as it
  // will be done on save
  auto nor = !(keyChanged || pathChanged);
  m_pPushButtonRegenCert->setEnabled(
      nor && m_pCheckBoxEnableCrypto->isChecked());
}

void SettingsDialog::on_m_pPushButtonRegenCert_clicked() {
  appConfig().generateCertificate(true);
}

void SettingsDialog::updateKeyLengthOnFile(const QString &path) {
  SslCertificate ssl;
  auto length = ssl.getCertKeyLength(path);
  auto index = m_pComboBoxKeyLength->findText(length);
  m_pComboBoxKeyLength->setCurrentIndex(index);
  // Also update what is in the appconfig to match the file itself
  appConfig().setTlsKeyLength(length);
}

void SettingsDialog::enableControls(bool enable) {
  m_pLineEditScreenName->setEnabled(enable);
  m_pSpinBoxPort->setEnabled(enable);
  m_pLineEditInterface->setEnabled(enable);
  m_pComboLogLevel->setEnabled(enable);
  m_pCheckBoxLogToFile->setEnabled(enable);
  m_pComboElevate->setEnabled(enable);
  m_pCheckBoxAutoHide->setEnabled(enable);
  m_pCheckBoxPreventSleep->setEnabled(enable);
  m_pCheckBoxMinimizeToTray->setEnabled(enable);
  m_pLineEditCertificatePath->setEnabled(enable);
  m_pComboBoxKeyLength->setEnabled(enable);
  m_pPushButtonBrowseCert->setEnabled(enable);
  m_pCheckBoxEnableCrypto->setEnabled(enable);
  m_pCheckBoxClientHostMode->setEnabled(enable);
  m_pCheckBoxServerClientMode->setEnabled(enable);
  m_pCheckBoxServiceEnabled->setEnabled(enable);
  m_pCheckBoxMinimizeOnClose->setEnabled(enable);

  m_pCheckBoxLanguageSync->setEnabled(enable && isClientMode());
  m_pCheckBoxScrollDirection->setEnabled(enable && isClientMode());

  m_pCheckBoxClientHostMode->setEnabled(
      enable && isClientMode() && appConfig().initiateConnectionFromServer());
  m_pCheckBoxServerClientMode->setEnabled(
      enable && !isClientMode() && appConfig().initiateConnectionFromServer());

  m_pLabelLogPath->setEnabled(enable && m_pCheckBoxLogToFile->isChecked());
  m_pLineEditLogFilename->setEnabled(
      enable && m_pCheckBoxLogToFile->isChecked());
  m_pButtonBrowseLog->setEnabled(enable && m_pCheckBoxLogToFile->isChecked());
  m_pLabelKeyLength->setEnabled(enable && m_pCheckBoxEnableCrypto->isChecked());
  m_pComboBoxKeyLength->setEnabled(
      enable && m_pCheckBoxEnableCrypto->isChecked());
  m_pLabelCertificate->setEnabled(
      enable && m_pCheckBoxEnableCrypto->isChecked());
  m_pLineEditCertificatePath->setEnabled(
      enable && m_pCheckBoxEnableCrypto->isChecked());
  m_pPushButtonBrowseCert->setEnabled(
      enable && m_pCheckBoxEnableCrypto->isChecked());

  if (enable) {
    updateTlsRegenerateButton();
  }

#if defined(Q_OS_WIN)
  m_pComboElevate->setCurrentIndex(static_cast<int>(appConfig().elevateMode()));
#else
  // elevate checkbox is only usable on ms windows.
  m_pLabelElevate->setEnabled(false);
  m_pComboElevate->setEnabled(false);
#endif

  setupSeurity();
}

void SettingsDialog::on_m_pScreenNameValidator_finished(const QString &error) {
  m_nameError = error;
}
