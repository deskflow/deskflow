/*
 * Deskflow -- mouse and keyboard sharing utility
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
#include "ui_SettingsDialog.h"

#include "gui/core/CoreProcess.h"
#include "gui/messages.h"
#include "gui/tls/TlsCertificate.h"
#include "gui/tls/TlsUtility.h"
#include "gui/validators/ScreenNameValidator.h"
#include "gui/validators/ValidationError.h"

#ifdef DESKFLOW_GUI_HOOK_HEADER
#include DESKFLOW_GUI_HOOK_HEADER
#endif

#include <QDir>
#include <QFileDialog>
#include <QMessageBox>

using namespace deskflow::gui;

SettingsDialog::SettingsDialog(
    QWidget *parent, AppConfig &appConfig, const IServerConfig &serverConfig, const CoreProcess &coreProcess
)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
      ui{std::make_unique<Ui::SettingsDialog>()},
      m_appConfig(appConfig),
      m_serverConfig(serverConfig),
      m_coreProcess(coreProcess),
      m_tlsUtility(appConfig)
{

  ui->setupUi(this);

  const auto folderIcon =
      QIcon::fromTheme(QIcon::ThemeIcon::DocumentOpen, QIcon(QStringLiteral(":/icons/64x64/folder.png")));
  ui->m_pPushButtonTlsCertPath->setIcon(folderIcon);
  ui->m_pButtonBrowseLog->setIcon(folderIcon);

  // force the first tab, since qt creator sets the active tab as the last one
  // the developer was looking at, and it's easy to accidentally save that.
  ui->m_pTabWidget->setCurrentIndex(0);

  loadFromConfig();
  m_wasOriginallySystemScope = m_appConfig.isActiveScopeSystem();
  updateControls();

  m_pScreenNameError = new validators::ValidationError(this);
  ui->m_pLineEditScreenName->setValidator(
      new validators::ScreenNameValidator(ui->m_pLineEditScreenName, m_pScreenNameError, &serverConfig.screens())
  );

  connect(ui->m_pCheckBoxEnableTls, &QCheckBox::toggled, this, &SettingsDialog::updateTlsControlsEnabled);

  connect(
      this, &SettingsDialog::shown, this,
      [this] {
        if (!m_appConfig.isActiveScopeWritable()) {
          showReadOnlyMessage();
        }
      },
      Qt::QueuedConnection
  );

#ifdef DESKFLOW_GUI_HOOK_SETTINGS
  DESKFLOW_GUI_HOOK_SETTINGS
#endif
}

//
// Auto-connect slots
//

void SettingsDialog::on_m_pCheckBoxLogToFile_stateChanged(int i)
{
  bool checked = i == 2;

  ui->m_pLabelLogPath->setEnabled(checked);
  ui->m_pLineEditLogFilename->setEnabled(checked);
  ui->m_pButtonBrowseLog->setEnabled(checked);
}

void SettingsDialog::on_m_pButtonBrowseLog_clicked()
{
  QString fileName = QFileDialog::getSaveFileName(
      this, tr("Save log file to..."), ui->m_pLineEditLogFilename->text(), "Logs (*.log *.txt)"
  );

  if (!fileName.isEmpty()) {
    ui->m_pLineEditLogFilename->setText(fileName);
  }
}

void SettingsDialog::on_m_pCheckBoxEnableTls_clicked(bool)
{
  updateTlsControlsEnabled();
}

void SettingsDialog::on_m_pRadioSystemScope_toggled(bool checked)
{
  // We only need to test the System scoped Radio as they are connected
  m_appConfig.setLoadFromSystemScope(checked);
  loadFromConfig();
  updateControls();

  if (isVisible() && !m_appConfig.isActiveScopeWritable()) {
    showReadOnlyMessage();
  }
}

void SettingsDialog::on_m_pPushButtonTlsCertPath_clicked()
{
  QString fileName = QFileDialog::getSaveFileName(
      this, tr("Select a TLS certificate to use..."), ui->m_pLineEditTlsCertPath->text(), "Cert (*.pem)", nullptr,
      QFileDialog::DontConfirmOverwrite
  );

  if (!fileName.isEmpty()) {
    ui->m_pLineEditTlsCertPath->setText(fileName);

    if (QFile(fileName).exists()) {
      updateKeyLengthOnFile(fileName);
    } else {
      qDebug("no tls certificate file at: %s", qUtf8Printable(fileName));
    }
  }
}

void SettingsDialog::on_m_pPushButtonTlsRegenCert_clicked()
{
  if (m_tlsUtility.generateCertificate()) {
    QMessageBox::information(this, tr("TLS Certificate Regenerated"), tr("TLS certificate regenerated successfully."));
  }
}

void SettingsDialog::on_m_pCheckBoxServiceEnabled_toggled(bool)
{
  updateControls();
}

//
// End of auto-connect slots
//

void SettingsDialog::showEvent(QShowEvent *event)
{
  QDialog::showEvent(event);
  Q_EMIT shown();
}

void SettingsDialog::showReadOnlyMessage()
{
  const auto activeScopeFilename = m_appConfig.scopes().activeFilePath();
  messages::showReadOnlySettings(this, activeScopeFilename);
}

void SettingsDialog::accept()
{
  if (!ui->m_pLineEditScreenName->hasAcceptableInput()) {
    QMessageBox::warning(this, tr("Invalid screen name"), m_pScreenNameError->message());
    return;
  }

  m_appConfig.setLoadFromSystemScope(ui->m_pRadioSystemScope->isChecked());
  m_appConfig.setScreenName(ui->m_pLineEditScreenName->text());
  m_appConfig.setPort(ui->m_pSpinBoxPort->value());
  m_appConfig.setNetworkInterface(ui->m_pLineEditInterface->text());
  m_appConfig.setLogLevel(ui->m_pComboLogLevel->currentIndex());
  m_appConfig.setLogToFile(ui->m_pCheckBoxLogToFile->isChecked());
  m_appConfig.setLogFilename(ui->m_pLineEditLogFilename->text());
  m_appConfig.setElevateMode(static_cast<ElevateMode>(ui->m_pComboElevate->currentIndex()));
  m_appConfig.setAutoHide(ui->m_pCheckBoxAutoHide->isChecked());
  m_appConfig.setEnableUpdateCheck(ui->m_pCheckBoxAutoUpdate->isChecked());
  m_appConfig.setPreventSleep(ui->m_pCheckBoxPreventSleep->isChecked());
  m_appConfig.setTlsCertPath(ui->m_pLineEditTlsCertPath->text());
  m_appConfig.setTlsKeyLength(ui->m_pComboBoxTlsKeyLength->currentText().toInt());
  m_appConfig.setTlsEnabled(ui->m_pCheckBoxEnableTls->isChecked());
  m_appConfig.setLanguageSync(ui->m_pCheckBoxLanguageSync->isChecked());
  m_appConfig.setInvertScrollDirection(ui->m_pCheckBoxScrollDirection->isChecked());
  m_appConfig.setEnableService(ui->m_pCheckBoxServiceEnabled->isChecked());
  m_appConfig.setCloseToTray(ui->m_pCheckBoxCloseToTray->isChecked());
  m_appConfig.setInvertConnection(ui->m_pInvertConnection->isChecked());

  QDialog::accept();
}

void SettingsDialog::reject()
{
  // restore original system scope value on reject.
  if (m_appConfig.isActiveScopeSystem() != m_wasOriginallySystemScope) {
    m_appConfig.setLoadFromSystemScope(m_wasOriginallySystemScope);
  }

  QDialog::reject();
}

void SettingsDialog::loadFromConfig()
{

  ui->m_pLineEditScreenName->setText(m_appConfig.screenName());
  ui->m_pSpinBoxPort->setValue(m_appConfig.port());
  ui->m_pLineEditInterface->setText(m_appConfig.networkInterface());
  ui->m_pComboLogLevel->setCurrentIndex(m_appConfig.logLevel());
  ui->m_pCheckBoxLogToFile->setChecked(m_appConfig.logToFile());
  ui->m_pLineEditLogFilename->setText(m_appConfig.logFilename());
  ui->m_pCheckBoxAutoHide->setChecked(m_appConfig.autoHide());
  ui->m_pCheckBoxPreventSleep->setChecked(m_appConfig.preventSleep());
  ui->m_pCheckBoxLanguageSync->setChecked(m_appConfig.languageSync());
  ui->m_pCheckBoxScrollDirection->setChecked(m_appConfig.invertScrollDirection());
  ui->m_pCheckBoxServiceEnabled->setChecked(m_appConfig.enableService());
  ui->m_pCheckBoxCloseToTray->setChecked(m_appConfig.closeToTray());
  ui->m_pComboElevate->setCurrentIndex(static_cast<int>(m_appConfig.elevateMode()));

  if (m_appConfig.enableUpdateCheck().has_value()) {
    ui->m_pCheckBoxAutoUpdate->setChecked(m_appConfig.enableUpdateCheck().value());
  } else {
    ui->m_pCheckBoxAutoUpdate->setChecked(false);
  }

  if (m_appConfig.isActiveScopeSystem()) {
    ui->m_pRadioSystemScope->setChecked(true);
  } else {
    ui->m_pRadioUserScope->setChecked(true);
  }

  ui->m_pInvertConnection->setChecked(m_appConfig.invertConnection());

  updateTlsControls();
}

void SettingsDialog::updateTlsControls()
{

  if (QFile(m_appConfig.tlsCertPath()).exists()) {
    updateKeyLengthOnFile(m_appConfig.tlsCertPath());
  } else {
    const auto keyLengthText = QString::number(m_appConfig.tlsKeyLength());
    ui->m_pComboBoxTlsKeyLength->setCurrentIndex(ui->m_pComboBoxTlsKeyLength->findText(keyLengthText));
  }

  const auto tlsEnabled = m_tlsUtility.isEnabled();
  const auto writable = m_appConfig.isActiveScopeWritable();

  ui->m_pCheckBoxEnableTls->setEnabled(writable);
  ui->m_pCheckBoxEnableTls->setChecked(writable && tlsEnabled);
  ui->m_pLineEditTlsCertPath->setText(m_appConfig.tlsCertPath());
}

void SettingsDialog::updateTlsControlsEnabled()
{
  const auto writable = m_appConfig.isActiveScopeWritable();
  const auto clientMode = m_appConfig.clientGroupChecked();
  const auto tlsChecked = ui->m_pCheckBoxEnableTls->isChecked();

  auto enabled = writable && tlsChecked && !clientMode;
  ui->m_pLabelTlsKeyLength->setEnabled(enabled);
  ui->m_pComboBoxTlsKeyLength->setEnabled(enabled);
  ui->m_pLabelTlsCert->setEnabled(enabled);
  ui->m_pLineEditTlsCertPath->setEnabled(enabled);
  ui->m_pPushButtonTlsCertPath->setEnabled(enabled);
  ui->m_pPushButtonTlsRegenCert->setEnabled(enabled);
}

bool SettingsDialog::isClientMode() const
{
  return m_coreProcess.mode() == deskflow::gui::CoreProcess::Mode::Client;
}

void SettingsDialog::updateKeyLengthOnFile(const QString &path)
{
  TlsCertificate ssl;
  if (!QFile(path).exists()) {
    qFatal("tls certificate file not found: %s", qUtf8Printable(path));
  }

  auto length = ssl.getCertKeyLength(path);
  auto index = ui->m_pComboBoxTlsKeyLength->findText(QString::number(length));
  ui->m_pComboBoxTlsKeyLength->setCurrentIndex(index);
  m_appConfig.setTlsKeyLength(length);
}

void SettingsDialog::updateControls()
{

#if defined(Q_OS_WIN)
  const auto serviceAvailable = true;
#else
  // service not supported on unix yet, so always disable.
  const auto serviceAvailable = false;
  ui->m_pGroupService->setTitle("Service (Windows only)");
#endif

  const bool writable = m_appConfig.isActiveScopeWritable();
  const bool serviceChecked = ui->m_pCheckBoxServiceEnabled->isChecked();
  const bool logToFile = ui->m_pCheckBoxLogToFile->isChecked();

  ui->m_pLineEditScreenName->setEnabled(writable);
  ui->m_pSpinBoxPort->setEnabled(writable);
  ui->m_pLineEditInterface->setEnabled(writable);
  ui->m_pComboLogLevel->setEnabled(writable);
  ui->m_pCheckBoxLogToFile->setEnabled(writable);
  ui->m_pCheckBoxAutoHide->setEnabled(writable);
  ui->m_pCheckBoxAutoUpdate->setEnabled(writable);
  ui->m_pCheckBoxPreventSleep->setEnabled(writable);
  ui->m_pLineEditTlsCertPath->setEnabled(writable);
  ui->m_pComboBoxTlsKeyLength->setEnabled(writable);
  ui->m_pCheckBoxCloseToTray->setEnabled(writable);

  ui->m_pCheckBoxServiceEnabled->setEnabled(writable && serviceAvailable);
  ui->m_pLabelElevate->setEnabled(writable && serviceChecked && serviceAvailable);
  ui->m_pComboElevate->setEnabled(writable && serviceChecked && serviceAvailable);

  ui->m_pCheckBoxLanguageSync->setEnabled(writable && isClientMode());
  ui->m_pCheckBoxScrollDirection->setEnabled(writable && isClientMode());

  ui->m_pLabelLogPath->setEnabled(writable && logToFile);
  ui->m_pLineEditLogFilename->setEnabled(writable && logToFile);
  ui->m_pButtonBrowseLog->setEnabled(writable && logToFile);

  updateTlsControls();
}

SettingsDialog::~SettingsDialog() = default;
