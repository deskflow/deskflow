/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 - 2026 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "SettingsDialog.h"
#include "common/PlatformInfo.h"
#include "ui_SettingsDialog.h"

#include "common/I18N.h"
#include "common/Settings.h"
#include "gui/Messages.h"
#include "gui/TlsUtility.h"
#include "gui/core/NetworkMonitor.h"

#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>

using namespace deskflow::gui;

SettingsDialog::SettingsDialog(QWidget *parent, const IServerConfig &serverConfig)
    : QDialog(parent),
      ui{std::make_unique<Ui::SettingsDialog>()},
      m_serverConfig(serverConfig)
{

  ui->setupUi(this);

  // hide advanced options on macOS and portable windows
  if (deskflow::platform::isMac() || (deskflow::platform::isWindows() && Settings::isPortableMode())) {
    ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->tabAdvanced));
  }

  // set up the language combo
  I18N::reDetectLanguages();
  ui->comboLanguage->addItems(I18N::detectedLanguages());
  ui->comboLanguage->setCurrentText(I18N::toNativeName(I18N::currentLanguage()));

  updateText();

  ui->comboTlsKeyLength->setItemIcon(0, QIcon::fromTheme(QStringLiteral("security-medium")));
  ui->comboTlsKeyLength->setItemIcon(1, QIcon::fromTheme(QIcon::ThemeIcon::SecurityHigh));
  ui->lblTlsCertInfo->setFixedSize(28, 28);

  ui->rbIconMono->setIcon(QIcon::fromTheme(QStringLiteral("%1-symbolic").arg(kRevFqdnName)));
  ui->rbIconColorful->setIcon(QIcon::fromTheme(kRevFqdnName));

  // force the first tab, since qt creator sets the active tab as the last one
  // the developer was looking at, and it's easy to accidentally save that.
  ui->tabWidget->setCurrentIndex(0);

  // Populate the list of IP addresses
  NetworkMonitor networkMonitor(this);
  for (const auto &address : networkMonitor.validAddresses()) {
    QString ipString = address;
    if (ui->comboInterface->findText(ipString) == -1) {
      ui->comboInterface->addItem(ipString, ipString);
    }
  }

  loadFromConfig();

  adjustSize();
  QApplication::processEvents();
  setFixedHeight(height());
  setWindowFlags((windowFlags() | Qt::CustomizeWindowHint) & ~Qt::WindowMinMaxButtonsHint);

  setButtonBoxEnabledButtons();
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
  connect(ui->buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, this, &SettingsDialog::loadFromConfig);
  connect(
      ui->buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this,
      &SettingsDialog::resetToDefault
  );

  connect(ui->groupSecurity, &QGroupBox::toggled, this, &SettingsDialog::updateTlsControlsEnabled);
  connect(ui->groupService, &QGroupBox::toggled, this, &SettingsDialog::updateControls);
  connect(ui->btnTlsRegenCert, &QPushButton::clicked, this, &SettingsDialog::regenCertificates);
  connect(ui->comboTlsKeyLength, &QComboBox::currentIndexChanged, this, &SettingsDialog::updateRequestedKeySize);
  connect(ui->btnTlsCertPath, &QPushButton::clicked, this, &SettingsDialog::browseCertificatePath);
  connect(ui->btnBrowseLog, &QPushButton::clicked, this, &SettingsDialog::browseLogPath);
  connect(ui->groupLogToFile, &QGroupBox::toggled, this, &SettingsDialog::setLogToFile);
  connect(ui->comboLogLevel, &QComboBox::currentIndexChanged, this, &SettingsDialog::logLevelChanged);
  connect(ui->comboLanguage, &QComboBox::currentTextChanged, this, [](const QString &lang) {
    const auto shortName = I18N::nativeTo639Name(lang);
    I18N::setLanguage(shortName);
  });

  // Connect modifiable controls
  connect(ui->rbIconMono, &QRadioButton::toggled, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->sbPort, &QSpinBox::valueChanged, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->comboLogLevel, &QComboBox::currentIndexChanged, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->comboInterface, &QComboBox::currentIndexChanged, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->comboTlsKeyLength, &QComboBox::currentIndexChanged, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->comboLanguage, &QComboBox::currentIndexChanged, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->cbAutoHide, &QCheckBox::toggled, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->cbPreventSleep, &QCheckBox::toggled, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->cbCloseToTray, &QCheckBox::toggled, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->cbElevateDaemon, &QCheckBox::toggled, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->cbAutoUpdate, &QCheckBox::toggled, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->cbGuiDebug, &QCheckBox::toggled, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->cbUseWlClipboard, &QCheckBox::toggled, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->cbShowVersion, &QCheckBox::toggled, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->cbRequireClientCert, &QCheckBox::toggled, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->groupLogToFile, &QGroupBox::toggled, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->groupService, &QGroupBox::toggled, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->groupSecurity, &QGroupBox::toggled, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->lineLogFilename, &QLineEdit::textChanged, this, &SettingsDialog::setButtonBoxEnabledButtons);
  connect(ui->lineTlsCertPath, &QLineEdit::textChanged, this, &SettingsDialog::setButtonBoxEnabledButtons);
}

void SettingsDialog::regenCertificates()
{
  if (TlsUtility::generateCertificate()) {
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
  ui->buttonBox->button(QDialogButtonBox::Save)->setToolTip(tr("Close and save changes"));
  ui->buttonBox->button(QDialogButtonBox::Cancel)->setToolTip(tr("Close and forget changes"));
  ui->buttonBox->button(QDialogButtonBox::Reset)->setToolTip(tr("Reset to stored values"));
  ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)->setToolTip(tr("Reset to default values"));
}

void SettingsDialog::accept()
{
  Settings::setValue(Settings::Core::Port, ui->sbPort->value());
  Settings::setValue(Settings::Core::Interface, ui->comboInterface->currentData());
  Settings::setValue(Settings::Log::Level, ui->comboLogLevel->currentIndex());
  Settings::setValue(Settings::Log::ToFile, ui->groupLogToFile->isChecked());
  Settings::setValue(Settings::Log::File, ui->lineLogFilename->text());
  Settings::setValue(Settings::Daemon::Elevate, ui->cbElevateDaemon->isChecked());
  Settings::setValue(Settings::Gui::Autohide, ui->cbAutoHide->isChecked());
  Settings::setValue(Settings::Gui::AutoUpdateCheck, ui->cbAutoUpdate->isChecked());
  Settings::setValue(Settings::Core::PreventSleep, ui->cbPreventSleep->isChecked());
  Settings::setValue(Settings::Security::Certificate, ui->lineTlsCertPath->text());
  Settings::setValue(Settings::Security::KeySize, ui->comboTlsKeyLength->currentText().toInt());
  Settings::setValue(Settings::Security::TlsEnabled, ui->groupSecurity->isChecked());
  Settings::setValue(Settings::Gui::CloseToTray, ui->cbCloseToTray->isChecked());
  Settings::setValue(Settings::Gui::SymbolicTrayIcon, ui->rbIconMono->isChecked());
  Settings::setValue(Settings::Security::CheckPeers, ui->cbRequireClientCert->isChecked());
  Settings::setValue(Settings::Core::Language, I18N::nativeTo639Name(ui->comboLanguage->currentText()));
  Settings::setValue(Settings::Log::GuiDebug, ui->cbGuiDebug->isChecked());
  Settings::setValue(Settings::Core::UseWlClipboard, ui->cbUseWlClipboard->isChecked());
  Settings::setValue(Settings::Gui::ShowVersionInTitle, ui->cbShowVersion->isChecked());

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
  ui->comboLogLevel->setCurrentIndex(Settings::value(Settings::Log::Level).toInt());
  ui->groupLogToFile->setChecked(Settings::value(Settings::Log::ToFile).toBool());
  ui->lineLogFilename->setText(Settings::value(Settings::Log::File).toString());
  ui->cbAutoHide->setChecked(Settings::value(Settings::Gui::Autohide).toBool());
  ui->cbPreventSleep->setChecked(Settings::value(Settings::Core::PreventSleep).toBool());
  ui->cbCloseToTray->setChecked(Settings::value(Settings::Gui::CloseToTray).toBool());
  ui->cbElevateDaemon->setChecked(Settings::value(Settings::Daemon::Elevate).toBool());
  ui->cbAutoUpdate->setChecked(Settings::value(Settings::Gui::AutoUpdateCheck).toBool());
  ui->cbGuiDebug->setChecked(Settings::value(Settings::Log::GuiDebug).toBool());
  ui->cbUseWlClipboard->setChecked(Settings::value(Settings::Core::UseWlClipboard).toBool());
  ui->cbShowVersion->setChecked(Settings::value(Settings::Gui::ShowVersionInTitle).toBool());

  const auto processMode = Settings::value(Settings::Core::ProcessMode).value<Settings::ProcessMode>();
  ui->groupService->setChecked(processMode == Settings::ProcessMode::Service);

  if (!deskflow::platform::isWindows())
    ui->groupService->setVisible(false);

  if (Settings::value(Settings::Gui::SymbolicTrayIcon).toBool())
    ui->rbIconMono->setChecked(true);
  else
    ui->rbIconColorful->setChecked(true);

  ui->lblDebugWarning->setVisible(Settings::value(Settings::Log::Level).toInt() > 4);

  ui->comboInterface->setCurrentText(Settings::value(Settings::Core::Interface).toString());
  if (ui->comboInterface->currentIndex() <= 0) {
    ui->comboInterface->setCurrentIndex(0);
    m_interfaceSetOnLoad = false;
  } else {
    m_interfaceSetOnLoad = true;
  }

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

  ui->lineTlsCertPath->setText(certificate);
  ui->cbRequireClientCert->setChecked(Settings::value(Settings::Security::CheckPeers).toBool());
  ui->groupSecurity->setChecked(TlsUtility::isEnabled());

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
  return Settings::value(Settings::Core::CoreMode) == Settings::CoreMode::Client;
}

void SettingsDialog::updateKeyLengthOnFile(const QString &path)
{
  if (!QFile(path).exists()) {
    qFatal("tls certificate file not found: %s", qUtf8Printable(path));
  }

  auto length = TlsUtility::getCertKeyLength(path);
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
  const bool logToFile = ui->groupLogToFile->isChecked();

  ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(writable);

  ui->sbPort->setEnabled(writable);
  ui->comboInterface->setEnabled(writable);
  ui->comboLogLevel->setEnabled(writable);
  ui->groupLogToFile->setEnabled(writable);
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

  // wl-clipboard support only works on wayland.
  // options should only be available when we are *not* running on wayland.
  if (deskflow::platform::isWayland()) {
    ui->cbUseWlClipboard->setEnabled(writable);
  } else if (ui->widgetWlClipboard->isVisibleTo(ui->tabAdvanced)) {
    ui->widgetWlClipboard->setVisible(false);
  }

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

bool SettingsDialog::isModified() const
{
  const auto processMode = Settings::value(Settings::Core::ProcessMode).value<Settings::ProcessMode>();
  const bool ignoreInterface = !m_interfaceSetOnLoad && (ui->comboInterface->currentIndex() == 0);

  bool modified =
      (ui->sbPort->value() != Settings::value(Settings::Core::Port).toInt()) ||
      (ui->comboLogLevel->currentIndex() != Settings::value(Settings::Log::Level).toInt()) ||
      (ui->groupLogToFile->isChecked() != Settings::value(Settings::Log::ToFile).toBool()) ||
      (ui->lineLogFilename->text() != Settings::value(Settings::Log::File).toString()) ||
      (ui->cbAutoHide->isChecked() != Settings::value(Settings::Gui::Autohide).toBool()) ||
      (ui->cbPreventSleep->isChecked() != Settings::value(Settings::Core::PreventSleep).toBool()) ||
      (ui->cbCloseToTray->isChecked() != Settings::value(Settings::Gui::CloseToTray).toBool()) ||
      (ui->cbElevateDaemon->isChecked() != Settings::value(Settings::Daemon::Elevate).toBool()) ||
      (ui->cbAutoUpdate->isChecked() != Settings::value(Settings::Gui::AutoUpdateCheck).toBool()) ||
      (ui->cbGuiDebug->isChecked() != Settings::value(Settings::Log::GuiDebug).toBool()) ||
      (ui->cbUseWlClipboard->isChecked() != Settings::value(Settings::Core::UseWlClipboard).toBool()) ||
      (ui->cbShowVersion->isChecked() != Settings::value(Settings::Gui::ShowVersionInTitle).toBool()) ||
      (ui->rbIconMono->isChecked() != Settings::value(Settings::Gui::SymbolicTrayIcon).toBool()) ||
      (ui->groupService->isChecked() != (processMode == Settings::ProcessMode::Service)) ||
      (ui->lineTlsCertPath->text() != Settings::value(Settings::Security::Certificate).toString()) ||
      (ui->comboTlsKeyLength->currentText() != Settings::value(Settings::Security::KeySize).toString()) ||
      (ui->groupSecurity->isChecked() != Settings::value(Settings::Security::TlsEnabled).toBool()) ||
      (ui->cbRequireClientCert->isChecked() != Settings::value(Settings::Security::CheckPeers).toBool()) ||
      (I18N::nativeTo639Name(ui->comboLanguage->currentText()) != Settings::value(Settings::Core::Language).toString());

  if (!ignoreInterface)
    modified = modified || ui->comboInterface->currentText() != Settings::value(Settings::Core::Interface).toString();
  return modified;
}

bool SettingsDialog::isDefault() const
{
  const auto processMode = Settings::defaultValue(Settings::Core::ProcessMode).value<Settings::ProcessMode>();

  return (
      (ui->sbPort->value() == Settings::defaultValue(Settings::Core::Port).toInt()) &&
      (ui->comboLogLevel->currentIndex() == Settings::defaultValue(Settings::Log::Level).toInt()) &&
      (ui->groupLogToFile->isChecked() == Settings::defaultValue(Settings::Log::ToFile).toBool()) &&
      (ui->lineLogFilename->text() == Settings::defaultValue(Settings::Log::File).toString()) &&
      (ui->cbAutoHide->isChecked() == Settings::defaultValue(Settings::Gui::Autohide).toBool()) &&
      (ui->cbPreventSleep->isChecked() == Settings::defaultValue(Settings::Core::PreventSleep).toBool()) &&
      (ui->cbCloseToTray->isChecked() == Settings::defaultValue(Settings::Gui::CloseToTray).toBool()) &&
      (ui->cbElevateDaemon->isChecked() == Settings::defaultValue(Settings::Daemon::Elevate).toBool()) &&
      (ui->cbAutoUpdate->isChecked() == Settings::defaultValue(Settings::Gui::AutoUpdateCheck).toBool()) &&
      (ui->cbGuiDebug->isChecked() == Settings::defaultValue(Settings::Log::GuiDebug).toBool()) &&
      (ui->cbUseWlClipboard->isChecked() == Settings::defaultValue(Settings::Core::UseWlClipboard).toBool()) &&
      (ui->cbShowVersion->isChecked() == Settings::defaultValue(Settings::Gui::ShowVersionInTitle).toBool()) &&
      (ui->rbIconMono->isChecked() == Settings::defaultValue(Settings::Gui::SymbolicTrayIcon).toBool()) &&
      (ui->groupService->isChecked() == (processMode == Settings::ProcessMode::Service)) &&
      (ui->comboInterface->currentIndex() == 0) &&
      (ui->lineTlsCertPath->text() == Settings::defaultValue(Settings::Security::Certificate).toString()) &&
      (ui->comboTlsKeyLength->currentText() == Settings::defaultValue(Settings::Security::KeySize).toString()) &&
      (ui->groupSecurity->isChecked() == Settings::defaultValue(Settings::Security::TlsEnabled).toBool()) &&
      (ui->cbRequireClientCert->isChecked() == Settings::defaultValue(Settings::Security::CheckPeers).toBool()) &&
      (ui->comboLanguage->currentText() == "English")
  );
}

void SettingsDialog::resetToDefault()
{
  ui->sbPort->setValue(Settings::defaultValue(Settings::Core::Port).toInt());
  ui->comboLogLevel->setCurrentIndex(Settings::defaultValue(Settings::Log::Level).toInt());
  ui->groupLogToFile->setChecked(Settings::defaultValue(Settings::Log::ToFile).toBool());
  ui->lineLogFilename->setText(Settings::defaultValue(Settings::Log::File).toString());
  ui->cbAutoHide->setChecked(Settings::defaultValue(Settings::Gui::Autohide).toBool());
  ui->cbPreventSleep->setChecked(Settings::defaultValue(Settings::Core::PreventSleep).toBool());
  ui->cbCloseToTray->setChecked(Settings::defaultValue(Settings::Gui::CloseToTray).toBool());
  ui->cbElevateDaemon->setChecked(Settings::defaultValue(Settings::Daemon::Elevate).toBool());
  ui->cbAutoUpdate->setChecked(Settings::defaultValue(Settings::Gui::AutoUpdateCheck).toBool());
  ui->cbGuiDebug->setChecked(Settings::defaultValue(Settings::Log::GuiDebug).toBool());
  ui->cbUseWlClipboard->setChecked(Settings::defaultValue(Settings::Core::UseWlClipboard).toBool());
  ui->cbShowVersion->setChecked(Settings::defaultValue(Settings::Gui::ShowVersionInTitle).toBool());

  const auto processMode = Settings::defaultValue(Settings::Core::ProcessMode).value<Settings::ProcessMode>();
  ui->groupService->setChecked(processMode == Settings::ProcessMode::Service);

  if (!deskflow::platform::isWindows())
    ui->groupService->setVisible(false);

  if (Settings::defaultValue(Settings::Gui::SymbolicTrayIcon).toBool())
    ui->rbIconMono->setChecked(true);
  else
    ui->rbIconColorful->setChecked(true);

  ui->lblDebugWarning->setVisible(false);

  ui->comboInterface->setCurrentIndex(0);

  qDebug() << "reset to default values";
  updateControls();
  setButtonBoxEnabledButtons();
}

void SettingsDialog::setButtonBoxEnabledButtons() const
{
  const bool modified = isModified();
  ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(modified);
  ui->buttonBox->button(QDialogButtonBox::Reset)->setEnabled(modified);
  ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)->setEnabled(!isDefault());
}

SettingsDialog::~SettingsDialog() = default;
