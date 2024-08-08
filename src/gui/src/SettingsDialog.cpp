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

#include "MainWindow.h"
#include "UpgradeDialog.h"
#include "gui/config/AppConfig.h"
#include "gui/constants.h"
#include "gui/tls/TlsCertificate.h"
#include "gui/tls/TlsUtility.h"
#include "validators/ScreenNameValidator.h"
#include "validators/ValidationError.h"

#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QtCore>
#include <QtGui>
#include <qglobal.h>

using namespace synergy::license;
using namespace synergy::gui;

const char *const kProProductName = "Synergy 1 Pro";

SettingsDialog::SettingsDialog(
    MainWindow *parent, AppConfig &appConfig, const IServerConfig &serverConfig,
    const License &license, const CoreProcess &coreProcess)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
      Ui::SettingsDialogBase(),
      m_appConfig(appConfig),
      m_serverConfig(serverConfig),
      m_license(license),
      m_coreProcess(coreProcess),
      m_tlsUtility(appConfig, license) {

  setupUi(this);

  // force the first tab, since qt creator sets the active tab as the last one
  // the developer was looking at, and it's easy to accidentally save that.
  m_pTabWidget->setCurrentIndex(0);

  loadFromConfig();
  m_wasOriginallySystemScope = m_appConfig.isActiveScopeSystem();
  updateControls();

  m_pScreenNameError = new validators::ValidationError(this);
  m_pLineEditScreenName->setValidator(new validators::ScreenNameValidator(
      m_pLineEditScreenName, m_pScreenNameError, &serverConfig.screens()));
}

//
// Auto-connect slots
//

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

void SettingsDialog::on_m_pCheckBoxEnableCrypto_clicked(bool) {
  updateTlsControlsEnabled();

  if (kEnableActivation && !m_tlsUtility.isAvailable()) {
    auto edition = m_license.productEdition();
    if (edition == Edition::kBasic) {
      UpgradeDialog upgradeDialog(this);
      upgradeDialog.showDialog(
          QString("Upgrade to %1 to enable TLS encryption.")
              .arg(kProProductName));
    }
  }
}

void SettingsDialog::on_m_pRadioSystemScope_toggled(bool checked) {
  // We only need to test the System scoped Radio as they are connected
  m_appConfig.setLoadFromSystemScope(checked);
  loadFromConfig();
  updateControls();
}

void SettingsDialog::on_m_pPushButtonBrowseCert_clicked() {
  QString fileName = QFileDialog::getSaveFileName(
      this, tr("Select a TLS certificate to use..."),
      m_pLineEditCertificatePath->text(), "Cert (*.pem)", nullptr,
      QFileDialog::DontConfirmOverwrite);

  if (!fileName.isEmpty()) {
    m_pLineEditCertificatePath->setText(fileName);

    if (QFile(fileName).exists()) {
      updateKeyLengthOnFile(fileName);
    } else {
      qDebug("no tls certificate file at: %s", qUtf8Printable(fileName));
    }
  }
  updateTlsRegenerateButton();
}

void SettingsDialog::on_m_pComboBoxKeyLength_currentIndexChanged(int index) {
  updateTlsRegenerateButton();
}

void SettingsDialog::on_m_pPushButtonRegenCert_clicked() {
  if (m_tlsUtility.generateCertificate()) {
    QMessageBox::information(
        this, tr("TLS Certificate Regenerated"),
        tr("TLS certificate regenerated successfully."));
  }
}

void SettingsDialog::on_m_pCheckBoxServiceEnabled_toggled(bool) {
  updateControls();
}

//
// End of auto-connect slots
//

void SettingsDialog::accept() {
  if (!m_pLineEditScreenName->hasAcceptableInput()) {
    QMessageBox::warning(
        this, tr("Invalid screen name"), m_pScreenNameError->message());
    return;
  }

  m_appConfig.setLoadFromSystemScope(m_pRadioSystemScope->isChecked());
  m_appConfig.setScreenName(m_pLineEditScreenName->text());
  m_appConfig.setPort(m_pSpinBoxPort->value());
  m_appConfig.setNetworkInterface(m_pLineEditInterface->text());
  m_appConfig.setLogLevel(m_pComboLogLevel->currentIndex());
  m_appConfig.setLogToFile(m_pCheckBoxLogToFile->isChecked());
  m_appConfig.setLogFilename(m_pLineEditLogFilename->text());
  m_appConfig.setElevateMode(
      static_cast<ElevateMode>(m_pComboElevate->currentIndex()));
  m_appConfig.setAutoHide(m_pCheckBoxAutoHide->isChecked());
  m_appConfig.setPreventSleep(m_pCheckBoxPreventSleep->isChecked());
  m_appConfig.setTlsCertPath(m_pLineEditCertificatePath->text());
  m_appConfig.setTlsKeyLength(m_pComboBoxKeyLength->currentText().toInt());
  m_appConfig.setTlsEnabled(m_pCheckBoxEnableCrypto->isChecked());
  m_appConfig.setLanguageSync(m_pCheckBoxLanguageSync->isChecked());
  m_appConfig.setInvertScrollDirection(m_pCheckBoxScrollDirection->isChecked());
  m_appConfig.setEnableService(m_pCheckBoxServiceEnabled->isChecked());
  m_appConfig.setCloseToTray(m_pCheckBoxCloseToTray->isChecked());
  m_appConfig.setInvertConnection(m_pInvertConnection->isChecked());

  QDialog::accept();
}

void SettingsDialog::reject() {
  // restore original system scope value on reject.
  if (m_appConfig.isActiveScopeSystem() != m_wasOriginallySystemScope) {
    m_appConfig.setLoadFromSystemScope(m_wasOriginallySystemScope);
  }

  QDialog::reject();
}

void SettingsDialog::loadFromConfig() {

  m_pLineEditScreenName->setText(m_appConfig.screenName());
  m_pSpinBoxPort->setValue(m_appConfig.port());
  m_pLineEditInterface->setText(m_appConfig.networkInterface());
  m_pComboLogLevel->setCurrentIndex(m_appConfig.logLevel());
  m_pCheckBoxLogToFile->setChecked(m_appConfig.logToFile());
  m_pLineEditLogFilename->setText(m_appConfig.logFilename());
  m_pCheckBoxAutoHide->setChecked(m_appConfig.autoHide());
  m_pCheckBoxPreventSleep->setChecked(m_appConfig.preventSleep());
  m_pLineEditCertificatePath->setText(m_appConfig.tlsCertPath());
  m_pCheckBoxEnableCrypto->setChecked(m_appConfig.tlsEnabled());
  m_pCheckBoxLanguageSync->setChecked(m_appConfig.languageSync());
  m_pCheckBoxScrollDirection->setChecked(m_appConfig.invertScrollDirection());
  m_pCheckBoxServiceEnabled->setChecked(m_appConfig.enableService());
  m_pCheckBoxCloseToTray->setChecked(m_appConfig.closeToTray());
  m_pComboElevate->setCurrentIndex(static_cast<int>(m_appConfig.elevateMode()));

  if (m_appConfig.isActiveScopeSystem()) {
    m_pRadioSystemScope->setChecked(true);
  } else {
    m_pRadioUserScope->setChecked(true);
  }

  m_pInvertConnection->setChecked(m_appConfig.invertConnection());
  m_pInvertConnection->setEnabled(
      m_license.productEdition() == Edition::kBusiness);

  updateTlsControls();
}

void SettingsDialog::updateTlsControls() {
  if (QFile(m_appConfig.tlsCertPath()).exists()) {
    updateKeyLengthOnFile(m_appConfig.tlsCertPath());
  } else {
    const auto keyLengthText = QString::number(m_appConfig.tlsKeyLength());
    m_pComboBoxKeyLength->setCurrentIndex(
        m_pComboBoxKeyLength->findText(keyLengthText));
  }

  m_pCheckBoxEnableCrypto->setChecked(m_appConfig.tlsEnabled());

  updateTlsControlsEnabled();
}

void SettingsDialog::updateTlsControlsEnabled() {
  const auto writable = m_appConfig.isActiveScopeWritable();
  const auto clientMode = m_appConfig.clientGroupChecked();
  const auto tlsAvailable = m_tlsUtility.isAvailableAndEnabled();
  const auto tlsChecked = m_pCheckBoxEnableCrypto->isChecked();

  auto enabled = writable && !clientMode && tlsAvailable && tlsChecked;

  qDebug(
      "tls enabled=%d, writable=%d, client=%d, available=%d, checked=%d",
      enabled, writable, clientMode, tlsAvailable, tlsChecked);

  m_pLabelKeyLength->setEnabled(enabled);
  m_pComboBoxKeyLength->setEnabled(enabled);
  m_pLabelCertificate->setEnabled(enabled);
  m_pLineEditCertificatePath->setEnabled(enabled);
  m_pPushButtonBrowseCert->setEnabled(enabled);
  m_pPushButtonRegenCert->setEnabled(enabled);
}

bool SettingsDialog::isClientMode() const {
  return m_coreProcess.mode() == MainWindow::CoreMode::Client;
}

void SettingsDialog::updateTlsRegenerateButton() {
  const auto writable = m_appConfig.isActiveScopeWritable();
  const auto keyLength = m_pComboBoxKeyLength->currentText().toInt();
  const auto path = m_pLineEditCertificatePath->text();
  const auto keyChanged = m_appConfig.tlsKeyLength() != keyLength;
  const auto pathChanged = m_appConfig.tlsCertPath() != path;
  const auto tlsEnabled = m_pCheckBoxEnableCrypto->isChecked();

  m_pPushButtonRegenCert->setEnabled(
      writable && tlsEnabled && (keyChanged || pathChanged));
}

void SettingsDialog::updateKeyLengthOnFile(const QString &path) {
  TlsCertificate ssl;
  if (!QFile(path).exists()) {
    qFatal("tls certificate file not found: %s", qUtf8Printable(path));
  }

  auto length = ssl.getCertKeyLength(path);
  auto index = m_pComboBoxKeyLength->findText(QString::number(length));
  m_pComboBoxKeyLength->setCurrentIndex(index);
  m_appConfig.setTlsKeyLength(length);
}

void SettingsDialog::updateControls() {

#if defined(Q_OS_WIN)
  const auto serviceAvailable = true;
#else
  // service not supported on unix yet, so always disable.
  const auto serviceAvailable = false;
  m_pGroupService->setTitle("Service (Windows only)");
#endif

  const bool writable = m_appConfig.isActiveScopeWritable();
  const bool serviceChecked = m_pCheckBoxServiceEnabled->isChecked();
  const bool logToFile = m_pCheckBoxLogToFile->isChecked();

  m_pLineEditScreenName->setEnabled(writable);
  m_pSpinBoxPort->setEnabled(writable);
  m_pLineEditInterface->setEnabled(writable);
  m_pComboLogLevel->setEnabled(writable);
  m_pCheckBoxLogToFile->setEnabled(writable);
  m_pCheckBoxAutoHide->setEnabled(writable);
  m_pCheckBoxPreventSleep->setEnabled(writable);
  m_pLineEditCertificatePath->setEnabled(writable);
  m_pComboBoxKeyLength->setEnabled(writable);
  m_pPushButtonBrowseCert->setEnabled(writable);
  m_pCheckBoxEnableCrypto->setEnabled(writable);
  m_pCheckBoxCloseToTray->setEnabled(writable);

  m_pCheckBoxServiceEnabled->setEnabled(writable && serviceAvailable);
  m_pLabelElevate->setEnabled(writable && serviceAvailable && serviceChecked);
  m_pComboElevate->setEnabled(writable && serviceAvailable && serviceChecked);

  m_pCheckBoxLanguageSync->setEnabled(writable && isClientMode());
  m_pCheckBoxScrollDirection->setEnabled(writable && isClientMode());

  m_pLabelLogPath->setEnabled(writable && logToFile);
  m_pLineEditLogFilename->setEnabled(writable && logToFile);
  m_pButtonBrowseLog->setEnabled(writable && logToFile);

  updateTlsControlsEnabled();
  updateTlsRegenerateButton();
  updateTlsControls();
}
