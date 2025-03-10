/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"

#include "common/DeskflowSettings.h"
#include "gui/core/CoreProcess.h"
#include "gui/messages.h"
#include "gui/tls/TlsCertificate.h"
#include "gui/tls/TlsUtility.h"

#include <QDir>
#include <QFileDialog>
#include <QMessageBox>

using namespace deskflow::gui;

SettingsDialog::SettingsDialog(QWidget *parent, const IServerConfig &serverConfig, const CoreProcess &coreProcess)
    : QDialog(parent),
      ui{std::make_unique<Ui::SettingsDialog>()},
      m_serverConfig(serverConfig),
      m_coreProcess(coreProcess),
      m_tlsUtility(this)
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
  m_wasOriginallySystemScope = DeskflowSettings::isSystemScope();
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
  DeskflowSettings::setScope(systemScope);
  loadFromConfig();
  updateControls();

  if (isVisible() && DeskflowSettings::isWritable()) {
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
  if (DeskflowSettings::isWritable())
    return;
  messages::showReadOnlySettings(this, DeskflowSettings::settingsFile());
}

void SettingsDialog::accept()
{
  DeskflowSettings::setScope(ui->rbScopeSystem->isChecked());
  DeskflowSettings::setValue(Settings::Core::Port, ui->sbPort->value());
  DeskflowSettings::setValue(Settings::Core::Interface, ui->lineInterface->text());
  DeskflowSettings::setValue(Settings::Log::Level, ui->comboLogLevel->currentIndex());
  DeskflowSettings::setValue(Settings::Log::ToFile, ui->cbLogToFile->isChecked());
  DeskflowSettings::setValue(Settings::Log::File, ui->lineLogFilename->text());
  DeskflowSettings::setValue(Settings::Core::ElevateMode, ui->comboElevate->currentIndex());
  DeskflowSettings::setValue(Settings::Gui::Autohide, ui->cbAutoHide->isChecked());
  DeskflowSettings::setValue(Settings::Gui::AutoUpdateCheck, ui->cbAutoUpdate->isChecked());
  DeskflowSettings::setValue(Settings::Core::PreventSleep, ui->cbPreventSleep->isChecked());
  DeskflowSettings::setValue(Settings::Security::Certificate, ui->lineTlsCertPath->text());
  DeskflowSettings::setValue(Settings::Security::KeySize, ui->comboTlsKeyLength->currentText().toInt());
  DeskflowSettings::setValue(Settings::Security::TlsEnabled, ui->groupSecurity->isChecked());
  DeskflowSettings::setValue(Settings::Client::LanguageSync, ui->cbLanguageSync->isChecked());
  DeskflowSettings::setValue(Settings::Client::InvertScrollDirection, ui->cbScrollDirection->isChecked());
  DeskflowSettings::setValue(Settings::Gui::CloseToTray, ui->cbCloseToTray->isChecked());
  DeskflowSettings::setValue(Settings::Gui::SymbolicTrayIcon, ui->rbIconMono->isChecked());
  DeskflowSettings::setValue(Settings::Security::CheckPeers, ui->cbRequireClientCert->isChecked());

  Settings::ProcessMode mode;
  if (ui->cbServiceEnabled->isChecked())
    mode = Settings::ProcessMode::Service;
  else
    mode = Settings::ProcessMode::Desktop;
  DeskflowSettings::setValue(Settings::Core::ProcessMode, mode);

  QDialog::accept();
}

void SettingsDialog::reject()
{
  // restore original system scope value on reject.
  if (DeskflowSettings::isSystemScope() != m_wasOriginallySystemScope) {
    DeskflowSettings::setScope(m_wasOriginallySystemScope);
  }

  QDialog::reject();
}

void SettingsDialog::loadFromConfig()
{

  ui->sbPort->setValue(DeskflowSettings::value(Settings::Core::Port).toInt());
  ui->lineInterface->setText(DeskflowSettings::value(Settings::Core::Interface).toString());
  ui->comboLogLevel->setCurrentIndex(DeskflowSettings::value(Settings::Log::Level).toInt());
  ui->cbLogToFile->setChecked(DeskflowSettings::value(Settings::Log::ToFile).toBool());
  ui->lineLogFilename->setText(DeskflowSettings::value(Settings::Log::File).toString());
  ui->cbAutoHide->setChecked(DeskflowSettings::value(Settings::Gui::Autohide).toBool());
  ui->cbPreventSleep->setChecked(DeskflowSettings::value(Settings::Core::PreventSleep).toBool());
  ui->cbLanguageSync->setChecked(DeskflowSettings::value(Settings::Client::LanguageSync).toBool());
  ui->cbScrollDirection->setChecked(DeskflowSettings::value(Settings::Client::InvertScrollDirection).toBool());
  ui->cbCloseToTray->setChecked(DeskflowSettings::value(Settings::Gui::CloseToTray).toBool());
  ui->comboElevate->setCurrentIndex(DeskflowSettings::value(Settings::Core::ElevateMode).toInt());

  ui->cbAutoUpdate->setChecked(DeskflowSettings::value(Settings::Gui::Autohide).toBool());

  if (DeskflowSettings::isSystemScope()) {
    ui->rbScopeSystem->setChecked(true);
  } else {
    ui->rbScopeUser->setChecked(true);
  }

  const auto processMode = DeskflowSettings::value(Settings::Core::ProcessMode).value<Settings::ProcessMode>();
  if (processMode == Settings::ProcessMode::Service) {
    ui->cbServiceEnabled->setChecked(true);
  }

  if (DeskflowSettings::value(Settings::Gui::SymbolicTrayIcon).toBool())
    ui->rbIconMono->setChecked(true);
  else
    ui->rbIconColorful->setChecked(true);

  qDebug() << "load from config done";
  updateTlsControls();
}

void SettingsDialog::updateTlsControls()
{
  const auto certificate = DeskflowSettings::value(Settings::Security::Certificate).toString();
  if (QFile(certificate).exists()) {
    updateKeyLengthOnFile(certificate);
  } else {
    const auto keyLengthText = DeskflowSettings::value(Settings::Security::KeySize).toString();
    ui->comboTlsKeyLength->setCurrentText(keyLengthText);
  }

  const auto tlsEnabled = DeskflowSettings::value(Settings::Security::TlsEnabled).toBool();
  const auto writable = DeskflowSettings::isWritable();
  const auto enabled = writable && tlsEnabled;

  ui->lineTlsCertPath->setText(certificate);
  ui->cbRequireClientCert->setChecked(DeskflowSettings::value(Settings::Security::CheckPeers).toBool());
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
  const auto writable = DeskflowSettings::isWritable();
  const auto clientMode =
      DeskflowSettings::value(Settings::Core::CoreMode).value<Settings::CoreMode>() == Settings::CoreMode::Client;
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
  return m_coreProcess.mode() == Settings::CoreMode::Client;
}

void SettingsDialog::updateKeyLengthOnFile(const QString &path)
{
  TlsCertificate ssl;
  if (!QFile(path).exists()) {
    qFatal("tls certificate file not found: %s", qUtf8Printable(path));
  }

  auto length = ssl.getCertKeyLength(path);
  ui->comboTlsKeyLength->setCurrentText(QString::number(length));
  DeskflowSettings::setValue(Settings::Security::KeySize, length);
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

  const bool writable = DeskflowSettings::isWritable();
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
