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

#include "UpgradeDialog.h"
#include "gui/constants.h"
#include "gui/core/CoreProcess.h"
#include "gui/tls/TlsCertificate.h"
#include "gui/tls/TlsUtility.h"
#include "gui/validators/ScreenNameValidator.h"
#include "gui/validators/ValidationError.h"

#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QtCore>
#include <QtGui>

using namespace synergy::license;
using namespace synergy::gui;

const char *const kProProductName = "Synergy 1 Pro";

SettingsDialog::SettingsDialog(
    QWidget *parent, IAppConfig &appConfig, const IServerConfig &serverConfig,
    const License &license, const CoreProcess &coreProcess)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
      Ui::SettingsDialogBase(),
      m_appConfig(appConfig),
      m_serverConfig(serverConfig),
      m_license(license),
      m_coreProcess(coreProcess),
      m_tlsUtility(appConfig, license) {

  qDebug("settings dialog ui setup");
  setupUi(this);

  // force the first tab, since qt creator sets the active tab as the last one
  // the developer was looking at, and it's easy to accidentally save that.
  m_pTabWidget->setCurrentIndex(0);

  loadFromConfig();
  updateControls();

  m_pScreenNameError = new validators::ValidationError(this);
  m_pLineEditScreenName->setValidator(new validators::ScreenNameValidator(
      m_pLineEditScreenName, m_pScreenNameError, &serverConfig.screens()));

  m_wasOriginallySystemScope = m_appConfig.isActiveScopeSystem();
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

void SettingsDialog::on_m_pCheckBoxEnableTls_clicked(bool) {
  updateTlsControlsEnabled();

  if (kEnableActivation && !m_tlsUtility.isAvailable()) {
    m_pCheckBoxEnableTls->setChecked(false);

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

void SettingsDialog::on_m_pPushButtonTlsCertPath_clicked() {
  QString fileName = QFileDialog::getSaveFileName(
      this, tr("Select a TLS certificate to use..."),
      m_pLineEditTlsCertPath->text(), "Cert (*.pem)", nullptr,
      QFileDialog::DontConfirmOverwrite);

  if (!fileName.isEmpty()) {
    m_pLineEditTlsCertPath->setText(fileName);

    if (QFile(fileName).exists()) {
      updateKeyLengthOnFile(fileName);
    } else {
      qDebug("no tls certificate file at: %s", qUtf8Printable(fileName));
    }
  }
  updateTlsRegenerateButton();
}

void SettingsDialog::on_m_pComboBoxTlsKeyLength_currentIndexChanged(int) {
  updateTlsRegenerateButton();
}

void SettingsDialog::on_m_pPushButtonTlsRegenCert_clicked() {
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
  m_appConfig.setTlsCertPath(m_pLineEditTlsCertPath->text());
  m_appConfig.setTlsKeyLength(m_pComboBoxTlsKeyLength->currentText().toInt());
  m_appConfig.setTlsEnabled(m_pCheckBoxEnableTls->isChecked());
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
  qDebug("loading settings from config");

  m_pLineEditScreenName->setText(m_appConfig.screenName());
  m_pSpinBoxPort->setValue(m_appConfig.port());
  m_pLineEditInterface->setText(m_appConfig.networkInterface());
  m_pComboLogLevel->setCurrentIndex(m_appConfig.logLevel());
  m_pCheckBoxLogToFile->setChecked(m_appConfig.logToFile());
  m_pLineEditLogFilename->setText(m_appConfig.logFilename());
  m_pCheckBoxAutoHide->setChecked(m_appConfig.autoHide());
  m_pCheckBoxPreventSleep->setChecked(m_appConfig.preventSleep());
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
    m_pComboBoxTlsKeyLength->setCurrentIndex(
        m_pComboBoxTlsKeyLength->findText(keyLengthText));
  }

  const auto tlsEnabled = m_tlsUtility.isAvailableAndEnabled();
  const auto writable = m_appConfig.isActiveScopeWritable();

  m_pCheckBoxEnableTls->setChecked(writable && tlsEnabled);
  m_pLineEditTlsCertPath->setText(m_appConfig.tlsCertPath());

  updateTlsControlsEnabled();
}

void SettingsDialog::updateTlsControlsEnabled() {
  const auto writable = m_appConfig.isActiveScopeWritable();
  const auto clientMode = m_appConfig.clientGroupChecked();
  const auto tlsChecked = m_pCheckBoxEnableTls->isChecked();

  auto enabled = writable && tlsChecked && !clientMode;
  m_pLabelTlsKeyLength->setEnabled(enabled);
  m_pComboBoxTlsKeyLength->setEnabled(enabled);
  m_pLabelTlsCert->setEnabled(enabled);
  m_pLineEditTlsCertPath->setEnabled(enabled);
  m_pPushButtonTlsCertPath->setEnabled(enabled);
  m_pPushButtonTlsRegenCert->setEnabled(enabled);
}

bool SettingsDialog::isClientMode() const {
  return m_coreProcess.mode() == synergy::gui::CoreProcess::Mode::Client;
}

void SettingsDialog::updateTlsRegenerateButton() {
  const auto writable = m_appConfig.isActiveScopeWritable();
  const auto keyLength = m_pComboBoxTlsKeyLength->currentText().toInt();
  const auto path = m_pLineEditTlsCertPath->text();
  const auto keyChanged = m_appConfig.tlsKeyLength() != keyLength;
  const auto pathChanged = m_appConfig.tlsCertPath() != path;
  const auto tlsEnabled = m_pCheckBoxEnableTls->isChecked();

  m_pPushButtonTlsRegenCert->setEnabled(
      writable && tlsEnabled && (keyChanged || pathChanged));
}

void SettingsDialog::updateKeyLengthOnFile(const QString &path) {
  TlsCertificate ssl;
  if (!QFile(path).exists()) {
    qFatal("tls certificate file not found: %s", qUtf8Printable(path));
  }

  auto length = ssl.getCertKeyLength(path);
  auto index = m_pComboBoxTlsKeyLength->findText(QString::number(length));
  m_pComboBoxTlsKeyLength->setCurrentIndex(index);
  m_appConfig.setTlsKeyLength(length);
}

void SettingsDialog::updateControls() {
  qDebug("updating settings dialog controls");

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
  m_pLineEditTlsCertPath->setEnabled(writable);
  m_pComboBoxTlsKeyLength->setEnabled(writable);
  m_pCheckBoxCloseToTray->setEnabled(writable);

  m_pCheckBoxServiceEnabled->setEnabled(writable && serviceAvailable);
  m_pLabelElevate->setEnabled(writable && serviceChecked && serviceAvailable);
  m_pComboElevate->setEnabled(writable && serviceChecked && serviceAvailable);

  m_pCheckBoxLanguageSync->setEnabled(writable && isClientMode());
  m_pCheckBoxScrollDirection->setEnabled(writable && isClientMode());

  m_pLabelLogPath->setEnabled(writable && logToFile);
  m_pLineEditLogFilename->setEnabled(writable && logToFile);
  m_pButtonBrowseLog->setEnabled(writable && logToFile);

  updateTlsControlsEnabled();
  updateTlsRegenerateButton();
  updateTlsControls();
}
