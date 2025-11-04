/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"

#include "common/I18N.h"
#include "common/Settings.h"
#include "gui/Messages.h"
#include "gui/core/CoreProcess.h"
#include "gui/tls/TlsCertificate.h"
#include "gui/tls/TlsUtility.h"

#include <QComboBox>
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

  // set up the language combo
  I18N::reDetectLanguages();
  ui->comboLanguage->addItems(I18N::detectedLanguages());
  ui->comboLanguage->setCurrentText(I18N::currentLanguage());

  updateText();

  ui->comboTlsKeyLength->setItemIcon(0, QIcon::fromTheme(QStringLiteral("security-medium")));
  ui->comboTlsKeyLength->setItemIcon(1, QIcon::fromTheme(QIcon::ThemeIcon::SecurityHigh));
  ui->lblTlsCertInfo->setFixedSize(28, 28);

  ui->rbIconMono->setIcon(QIcon::fromTheme(QStringLiteral("%1-symbolic").arg(kRevFqdnName)));
  ui->rbIconColorful->setIcon(QIcon::fromTheme(kRevFqdnName));

  // force the first tab, since qt creator sets the active tab as the last one
  // the developer was looking at, and it's easy to accidentally save that.
  ui->tabWidget->setCurrentIndex(0);

  loadFromConfig();

  adjustSize();
  QApplication::processEvents();
  setFixedHeight(height());
  setWindowFlags((windowFlags() | Qt::CustomizeWindowHint) & ~Qt::WindowMinMaxButtonsHint);

  initConnections();
}

void SettingsDialog::changeEvent(QEvent *e)
{
  QDialog::changeEvent(e);
  if (e->type() == QEvent::LanguageChange) {
    ui->retranslateUi(this);
    updateText();
  }
}

void SettingsDialog::initConnections() const
{
  connect(this, &SettingsDialog::shown, this, &SettingsDialog::showReadOnlyMessage, Qt::QueuedConnection);

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

  connect(ui->groupSecurity, &QGroupBox::toggled, this, &SettingsDialog::updateTlsControlsEnabled);
  connect(ui->groupService, &QGroupBox::toggled, this, &SettingsDialog::updateControls);
  connect(ui->btnTlsRegenCert, &QPushButton::clicked, this, &SettingsDialog::regenCertificates);
  connect(ui->comboTlsKeyLength, &QComboBox::currentIndexChanged, this, &SettingsDialog::updateRequestedKeySize);
  connect(ui->btnTlsCertPath, &QPushButton::clicked, this, &SettingsDialog::browseCertificatePath);
  connect(ui->btnBrowseLog, &QPushButton::clicked, this, &SettingsDialog::browseLogPath);
  connect(ui->cbLogToFile, &QCheckBox::toggled, this, &SettingsDialog::setLogToFile);
  connect(ui->comboLogLevel, &QComboBox::currentIndexChanged, this, &SettingsDialog::logLevelChanged);
  connect(ui->comboLanguage, &QComboBox::currentTextChanged, I18N::instance(), &I18N::setLanguage);
}

void SettingsDialog::regenCertificates()
{
  if (m_tlsUtility.generateCertificate()) {
    QMessageBox::information(this, tr("TLS Certificate Regenerated"), tr("TLS certificate regenerated successfully."));
    const auto certificate = Settings::value(Settings::Security::Certificate).toString();
    updateKeyLengthOnFile(certificate);
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

void SettingsDialog::showEvent(QShowEvent *event)
{
  QDialog::showEvent(event);
  Q_EMIT shown();
}

void SettingsDialog::showReadOnlyMessage()
{
  if (Settings::isWritable())
    return;
  messages::showReadOnlySettings(this, Settings::settingsFile());
}

void SettingsDialog::updateText()
{
  // Set Tooltip for the logLevel Items
  ui->comboLogLevel->setItemData(0, tr("Required messages"), Qt::ToolTipRole);
  ui->comboLogLevel->setItemData(1, tr("Non-fatal errors"), Qt::ToolTipRole);
  ui->comboLogLevel->setItemData(2, tr("General warnings"), Qt::ToolTipRole);
  ui->comboLogLevel->setItemData(3, tr("Notable events"), Qt::ToolTipRole);
  ui->comboLogLevel->setItemData(4, tr("General events [Default]"), Qt::ToolTipRole);
  ui->comboLogLevel->setItemData(5, tr("Debug entries"), Qt::ToolTipRole);
  ui->comboLogLevel->setItemData(6, tr("More debug output"), Qt::ToolTipRole);
  ui->comboLogLevel->setItemData(7, tr("Verbose debug output"), Qt::ToolTipRole);
}

void SettingsDialog::accept()
{
  Settings::setValue(Settings::Core::Port, ui->sbPort->value());
  Settings::setValue(Settings::Core::Interface, ui->lineInterface->text());
  Settings::setValue(Settings::Log::Level, ui->comboLogLevel->currentIndex());
  Settings::setValue(Settings::Log::ToFile, ui->cbLogToFile->isChecked());
  Settings::setValue(Settings::Log::File, ui->lineLogFilename->text());
  Settings::setValue(Settings::Daemon::Elevate, ui->cbElevateDaemon->isChecked());
  Settings::setValue(Settings::Gui::Autohide, ui->cbAutoHide->isChecked());
  Settings::setValue(Settings::Gui::AutoUpdateCheck, ui->cbAutoUpdate->isChecked());
  Settings::setValue(Settings::Core::PreventSleep, ui->cbPreventSleep->isChecked());
  Settings::setValue(Settings::Security::Certificate, ui->lineTlsCertPath->text());
  Settings::setValue(Settings::Security::KeySize, ui->comboTlsKeyLength->currentText().toInt());
  Settings::setValue(Settings::Security::TlsEnabled, ui->groupSecurity->isChecked());
  Settings::setValue(Settings::Client::LanguageSync, ui->cbLanguageSync->isChecked());
  Settings::setValue(Settings::Client::InvertScrollDirection, ui->cbScrollDirection->isChecked());
  Settings::setValue(Settings::Gui::CloseToTray, ui->cbCloseToTray->isChecked());
  Settings::setValue(Settings::Gui::SymbolicTrayIcon, ui->rbIconMono->isChecked());
  Settings::setValue(Settings::Security::CheckPeers, ui->cbRequireClientCert->isChecked());
  Settings::setValue(Settings::Client::ScrollSpeed, ui->sbScrollSpeed->value());
  Settings::setValue(Settings::Core::Language, ui->comboLanguage->currentText());
  Settings::setValue(Settings::Log::GuiDebug, ui->cbGuiDebug->isChecked());

  Settings::ProcessMode mode;
  if (ui->groupService->isChecked())
    mode = Settings::ProcessMode::Service;
  else
    mode = Settings::ProcessMode::Desktop;
  Settings::setValue(Settings::Core::ProcessMode, mode);

  QDialog::accept();
}

void SettingsDialog::loadFromConfig()
{
  ui->sbPort->setValue(Settings::value(Settings::Core::Port).toInt());
  ui->lineInterface->setText(Settings::value(Settings::Core::Interface).toString());
  ui->comboLogLevel->setCurrentIndex(Settings::value(Settings::Log::Level).toInt());
  ui->cbLogToFile->setChecked(Settings::value(Settings::Log::ToFile).toBool());
  ui->lineLogFilename->setText(Settings::value(Settings::Log::File).toString());
  ui->cbAutoHide->setChecked(Settings::value(Settings::Gui::Autohide).toBool());
  ui->cbPreventSleep->setChecked(Settings::value(Settings::Core::PreventSleep).toBool());
  ui->cbLanguageSync->setChecked(Settings::value(Settings::Client::LanguageSync).toBool());
  ui->cbScrollDirection->setChecked(Settings::value(Settings::Client::InvertScrollDirection).toBool());
  ui->cbCloseToTray->setChecked(Settings::value(Settings::Gui::CloseToTray).toBool());
  ui->cbElevateDaemon->setChecked(Settings::value(Settings::Daemon::Elevate).toBool());
  ui->cbAutoUpdate->setChecked(Settings::value(Settings::Gui::AutoUpdateCheck).toBool());
  ui->sbScrollSpeed->setValue(Settings::value(Settings::Client::ScrollSpeed).toInt());
  ui->cbGuiDebug->setChecked(Settings::value(Settings::Log::GuiDebug).toBool());

  const auto processMode = Settings::value(Settings::Core::ProcessMode).value<Settings::ProcessMode>();
  ui->groupService->setChecked(processMode == Settings::ProcessMode::Service);

#ifndef Q_OS_WIN
  ui->groupService->setVisible(false);
#endif

  if (Settings::value(Settings::Gui::SymbolicTrayIcon).toBool())
    ui->rbIconMono->setChecked(true);
  else
    ui->rbIconColorful->setChecked(true);

  ui->lblDebugWarning->setVisible(Settings::value(Settings::Log::Level).toInt() > 4);

  qDebug() << "load from config done";
  updateControls();
}

void SettingsDialog::updateTlsControls()
{
  const auto certificate = Settings::value(Settings::Security::Certificate).toString();
  if (QFile(certificate).exists()) {
    updateKeyLengthOnFile(certificate);
  }

  ui->comboTlsKeyLength->setCurrentText(Settings::value(Settings::Security::KeySize).toString());

  const auto tlsEnabled = Settings::value(Settings::Security::TlsEnabled).toBool();

  ui->lineTlsCertPath->setText(certificate);
  ui->cbRequireClientCert->setChecked(Settings::value(Settings::Security::CheckPeers).toBool());
  ui->groupSecurity->setChecked(tlsEnabled);

  ui->groupSecurity->setEnabled(Settings::isWritable());

  updateTlsControlsEnabled();
}

void SettingsDialog::updateTlsControlsEnabled()
{
  const auto writable = Settings::isWritable();
  const auto tlsChecked = ui->groupSecurity->isChecked();

  auto enabled = writable && tlsChecked;
  ui->lblTlsKeyLength->setEnabled(enabled);
  ui->comboTlsKeyLength->setEnabled(enabled);
  ui->lblTlsCert->setEnabled(enabled);
  ui->widgetTlsCert->setEnabled(enabled);
  ui->btnTlsRegenCert->setEnabled(enabled);
  ui->cbRequireClientCert->setEnabled(enabled && !isClientMode());
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
  auto labelIcon = QPixmap(QIcon::fromTheme(QIcon::ThemeIcon::SecurityLow).pixmap(24, 24));
  if (length == 2048)
    labelIcon = QPixmap(QIcon::fromTheme(QStringLiteral("security-medium")).pixmap(24, 24));
  if (length == 4096)
    labelIcon = QPixmap(QIcon::fromTheme(QIcon::ThemeIcon::SecurityHigh).pixmap(24, 24));

  ui->lblTlsCertInfo->setPixmap(labelIcon);
  ui->lblTlsCertInfo->setToolTip(QStringLiteral("Key length: %1 bits").arg(QString::number(length)));
}

void SettingsDialog::updateControls()
{
  const bool writable = Settings::isWritable();
  const bool serviceChecked = ui->groupService->isChecked();
  const bool logToFile = ui->cbLogToFile->isChecked();

  ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(writable);

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

  // Portable mode only ever applies to Windows.
  // Daemon options should only be available on Windows when *not* in portable mode.
  if (!Settings::isPortableMode()) {
    ui->groupService->setEnabled(writable);
    ui->cbElevateDaemon->setEnabled(writable && serviceChecked);
  } else if (ui->groupService->isVisibleTo(ui->tabAdvanced)) {
    ui->groupService->setVisible(false);
  }

  ui->groupClientOptions->setEnabled(writable && isClientMode());

  ui->widgetLogFilename->setEnabled(writable && logToFile);

  updateTlsControls();
}

void SettingsDialog::updateRequestedKeySize() const
{
  if (ui->comboTlsKeyLength->currentText() == Settings::value(Settings::Security::KeySize).toString())
    return;
  Settings::setValue(Settings::Security::KeySize, ui->comboTlsKeyLength->currentText());
}

void SettingsDialog::logLevelChanged()
{
  ui->lblDebugWarning->setVisible(ui->comboLogLevel->currentIndex() > 4);
}

SettingsDialog::~SettingsDialog() = default;
