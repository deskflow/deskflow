/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"

#include "gui/core/CoreProcess.h"
#include "gui/messages.h"
#include "gui/tls/TlsCertificate.h"
#include "gui/tls/TlsUtility.h"

#include <QDir>
#include <QFileDialog>
#include <QMessageBox>

using namespace deskflow::gui;

SettingsDialog::SettingsDialog(
    QWidget *parent, AppConfig &appConfig, const IServerConfig &serverConfig, const CoreProcess &coreProcess
)
    : QDialog(parent),
      ui{std::make_unique<Ui::SettingsDialog>()},
      m_appConfig(appConfig),
      m_serverConfig(serverConfig),
      m_coreProcess(coreProcess),
      m_tlsUtility(appConfig)
{

  ui->setupUi(this);

  ui->comboTlsKeyLength->setItemIcon(0, QIcon::fromTheme(QIcon::ThemeIcon::SecurityLow));
  ui->comboTlsKeyLength->setItemIcon(1, QIcon::fromTheme(QStringLiteral("security-medium")));
  ui->comboTlsKeyLength->setItemIcon(2, QIcon::fromTheme(QIcon::ThemeIcon::SecurityHigh));

  ui->rbIconMono->setIcon(QIcon::fromTheme(QStringLiteral("deskflow-symbolic")));
  ui->rbIconColorful->setIcon(QIcon::fromTheme(QStringLiteral("deskflow")));

  // force the first tab, since qt creator sets the active tab as the last one
  // the developer was looking at, and it's easy to accidentally save that.
  ui->tabWidget->setCurrentIndex(0);

  loadFromConfig();
  m_wasOriginallySystemScope = m_appConfig.isActiveScopeSystem();
  updateControls();

  adjustSize();
  QApplication::processEvents();
  setFixedHeight(height());
  setWindowFlags((windowFlags() | Qt::CustomizeWindowHint) & ~Qt::WindowMinMaxButtonsHint);

  initConnections();
}

void SettingsDialog::initConnections()
{
  connect(this, &SettingsDialog::shown, this, &SettingsDialog::showReadOnlyMessage, Qt::QueuedConnection);

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);

  connect(ui->groupSecurity, &QGroupBox::toggled, this, &SettingsDialog::updateTlsControlsEnabled);
  connect(ui->cbServiceEnabled, &QCheckBox::toggled, this, &SettingsDialog::updateControls);
  connect(ui->btnTlsRegenCert, &QPushButton::clicked, this, &SettingsDialog::regenCertificates);
  connect(ui->btnTlsCertPath, &QPushButton::clicked, this, &SettingsDialog::browseCertificatePath);
  connect(ui->btnBrowseLog, &QPushButton::clicked, this, &SettingsDialog::browseLogPath);
  connect(ui->cbLogToFile, &QCheckBox::toggled, this, &SettingsDialog::setLogToFile);

  // We only need to test the System scoped Radio as they are connected
  connect(ui->rbScopeSystem, &QRadioButton::toggled, this, &SettingsDialog::setSystemScope);
}

void SettingsDialog::regenCertificates()
{
  if (m_tlsUtility.generateCertificate()) {
    QMessageBox::information(this, tr("TLS Certificate Regenerated"), tr("TLS certificate regenerated successfully."));
  }
}

void SettingsDialog::browseCertificatePath()
{
  QString fileName = QFileDialog::getSaveFileName(
      this, tr("Select a TLS certificate to use..."), ui->lineTlsCertPath->text(), "Cert (*.pem)", nullptr,
      QFileDialog::DontConfirmOverwrite
  );

  if (!fileName.isEmpty()) {
    ui->lineTlsCertPath->setText(fileName);

    if (QFile(fileName).exists()) {
      updateKeyLengthOnFile(fileName);
    } else {
      qDebug("no tls certificate file at: %s", qUtf8Printable(fileName));
    }
  }
}

void SettingsDialog::browseLogPath()
{
  QString fileName =
      QFileDialog::getSaveFileName(this, tr("Save log file to..."), ui->lineLogFilename->text(), "Logs (*.log *.txt)");

  if (!fileName.isEmpty()) {
    ui->lineLogFilename->setText(fileName);
  }
}

void SettingsDialog::setLogToFile(bool logToFile)
{
  ui->widgetLogFilename->setEnabled(logToFile);
}

void SettingsDialog::setSystemScope(bool systemScope)
{
  m_appConfig.setLoadFromSystemScope(systemScope);
  loadFromConfig();
  updateControls();

  if (isVisible() && !m_appConfig.isActiveScopeWritable()) {
    showReadOnlyMessage();
  }
}

void SettingsDialog::showEvent(QShowEvent *event)
{
  QDialog::showEvent(event);
  Q_EMIT shown();
}

void SettingsDialog::showReadOnlyMessage()
{
  if (m_appConfig.isActiveScopeWritable())
    return;
  const auto activeScopeFilename = m_appConfig.scopes().activeFilePath();
  messages::showReadOnlySettings(this, activeScopeFilename);
}

void SettingsDialog::accept()
{
  m_appConfig.setLoadFromSystemScope(ui->rbScopeSystem->isChecked());
  m_appConfig.setPort(ui->sbPort->value());
  m_appConfig.setNetworkInterface(ui->lineInterface->text());
  m_appConfig.setLogLevel(ui->comboLogLevel->currentIndex());
  m_appConfig.setLogToFile(ui->cbLogToFile->isChecked());
  m_appConfig.setLogFilename(ui->lineLogFilename->text());
  m_appConfig.setElevateMode(static_cast<ElevateMode>(ui->comboElevate->currentIndex()));
  m_appConfig.setAutoHide(ui->cbAutoHide->isChecked());
  m_appConfig.setEnableUpdateCheck(ui->cbAutoUpdate->isChecked());
  m_appConfig.setPreventSleep(ui->cbPreventSleep->isChecked());
  m_appConfig.setTlsCertPath(ui->lineTlsCertPath->text());
  m_appConfig.setTlsKeyLength(ui->comboTlsKeyLength->currentText().toInt());
  m_appConfig.setTlsEnabled(ui->groupSecurity->isChecked());
  m_appConfig.setLanguageSync(ui->cbLanguageSync->isChecked());
  m_appConfig.setInvertScrollDirection(ui->cbScrollDirection->isChecked());
  m_appConfig.setEnableService(ui->cbServiceEnabled->isChecked());
  m_appConfig.setCloseToTray(ui->cbCloseToTray->isChecked());
  m_appConfig.setColorfulTrayIcon(ui->rbIconColorful->isChecked());
  m_appConfig.setRequireClientCerts(ui->cbRequireClientCert->isChecked());

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

  ui->sbPort->setValue(m_appConfig.port());
  ui->lineInterface->setText(m_appConfig.networkInterface());
  ui->comboLogLevel->setCurrentIndex(m_appConfig.logLevel());
  ui->cbLogToFile->setChecked(m_appConfig.logToFile());
  ui->lineLogFilename->setText(m_appConfig.logFilename());
  ui->cbAutoHide->setChecked(m_appConfig.autoHide());
  ui->cbPreventSleep->setChecked(m_appConfig.preventSleep());
  ui->cbLanguageSync->setChecked(m_appConfig.languageSync());
  ui->cbScrollDirection->setChecked(m_appConfig.invertScrollDirection());
  ui->cbServiceEnabled->setChecked(m_appConfig.enableService());
  ui->cbCloseToTray->setChecked(m_appConfig.closeToTray());
  ui->comboElevate->setCurrentIndex(static_cast<int>(m_appConfig.elevateMode()));

  if (m_appConfig.enableUpdateCheck().has_value()) {
    ui->cbAutoUpdate->setChecked(m_appConfig.enableUpdateCheck().value());
  } else {
    ui->cbAutoUpdate->setChecked(false);
  }

  if (m_appConfig.isActiveScopeSystem()) {
    ui->rbScopeSystem->setChecked(true);
  } else {
    ui->rbScopeUser->setChecked(true);
  }

  if (m_appConfig.colorfulTrayIcon())
    ui->rbIconColorful->setChecked(true);
  else
    ui->rbIconMono->setChecked(true);

  qDebug() << "load from config done";
  updateTlsControls();
}

void SettingsDialog::updateTlsControls()
{

  if (QFile(m_appConfig.tlsCertPath()).exists()) {
    updateKeyLengthOnFile(m_appConfig.tlsCertPath());
  } else {
    const auto keyLengthText = QString::number(m_appConfig.tlsKeyLength());
    ui->comboTlsKeyLength->setCurrentIndex(ui->comboTlsKeyLength->findText(keyLengthText));
  }

  const auto tlsEnabled = m_tlsUtility.isEnabled();
  const auto writable = m_appConfig.isActiveScopeWritable();
  const auto enabled = writable && tlsEnabled;

  ui->lineTlsCertPath->setText(m_appConfig.tlsCertPath());
  ui->cbRequireClientCert->setChecked(m_appConfig.requireClientCerts());
  ui->groupSecurity->setChecked(tlsEnabled);

  ui->groupSecurity->setEnabled(writable);
  ui->comboTlsKeyLength->setEnabled(enabled);
  ui->widgetTlsCert->setEnabled(enabled);
  ui->lblTlsKeyLength->setEnabled(enabled);
  ui->btnTlsRegenCert->setEnabled(enabled);
  ui->cbRequireClientCert->setEnabled(enabled);
}

void SettingsDialog::updateTlsControlsEnabled()
{
  const auto writable = m_appConfig.isActiveScopeWritable();
  const auto clientMode = m_appConfig.clientGroupChecked();
  const auto tlsChecked = ui->groupSecurity->isChecked();

  auto enabled = writable && tlsChecked && !clientMode;
  ui->lblTlsKeyLength->setEnabled(enabled);
  ui->comboTlsKeyLength->setEnabled(enabled);
  ui->lblTlsCert->setEnabled(enabled);
  ui->widgetTlsCert->setEnabled(enabled);
  ui->btnTlsRegenCert->setEnabled(enabled);
  ui->cbRequireClientCert->setEnabled(enabled);
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
  auto index = ui->comboTlsKeyLength->findText(QString::number(length));
  ui->comboTlsKeyLength->setCurrentIndex(index);
  m_appConfig.setTlsKeyLength(length);
}

void SettingsDialog::updateControls()
{

#if defined(Q_OS_WIN)
  const auto serviceAvailable = true;
#else
  // service not supported on unix yet, so always disable.
  const auto serviceAvailable = false;
  ui->groupService->setTitle("Service (Windows only)");
#endif

  const bool writable = m_appConfig.isActiveScopeWritable();
  const bool serviceChecked = ui->cbServiceEnabled->isChecked();
  const bool logToFile = ui->cbLogToFile->isChecked();

  ui->sbPort->setEnabled(writable);
  ui->lineInterface->setEnabled(writable);
  ui->comboLogLevel->setEnabled(writable);
  ui->cbLogToFile->setEnabled(writable);
  ui->cbAutoHide->setEnabled(writable);
  ui->cbAutoUpdate->setEnabled(writable);
  ui->cbPreventSleep->setEnabled(writable);
  ui->lineTlsCertPath->setEnabled(writable);
  ui->comboTlsKeyLength->setEnabled(writable);
  ui->cbCloseToTray->setEnabled(writable);

  ui->cbServiceEnabled->setEnabled(writable && serviceAvailable);
  ui->widgetElevate->setEnabled(writable && serviceChecked && serviceAvailable);

  ui->cbLanguageSync->setEnabled(writable && isClientMode());
  ui->cbScrollDirection->setEnabled(writable && isClientMode());

  ui->widgetLogFilename->setEnabled(writable && logToFile);

  updateTlsControls();
}

SettingsDialog::~SettingsDialog() = default;
