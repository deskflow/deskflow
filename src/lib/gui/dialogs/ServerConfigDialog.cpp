/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ServerConfigDialog.h"
#include "ui_ServerConfigDialog.h"

#include "common/Constants.h"
#include "common/NetworkProtocol.h"
#include "common/PlatformInfo.h"
#include "common/Settings.h"
#include "common/SharingConstants.h"
#include "dialogs/ActionDialog.h"
#include "dialogs/HotkeyDialog.h"
#include "dialogs/ScreenSettingsDialog.h"

#include "gui/StyleUtils.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QSignalBlocker>

using enum ScreenConfig::SwitchCorner;

ServerConfigDialog::ServerConfigDialog(QWidget *parent, ServerConfig &config)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
      ui{std::make_unique<Ui::ServerConfigDialog>()},
      m_columns{Settings::value(Settings::Server::GridWidth).toInt()},
      m_rows{Settings::value(Settings::Server::GridHeight).toInt()},
      m_originalServerConfig(config),
      m_originalServerConfigIsExternal(config.useExternalConfig()),
      m_originalServerConfigUsesExternalFile(config.configFile()),
      m_serverConfig(config),
      m_screenSetupModel(m_serverConfig.screens(), m_columns, m_rows)
{
  ui->setupUi(this);

  loadFromConfig();

  ui->lblRemoveScreen->setPixmap(deskflow::gui::themedPixmap(QStringLiteral("user-trash"), 64));
  ui->lblNewScreen->setEnabled(!model().isFull());
  ui->lblNewScreen->setPixmap(deskflow::gui::themedPixmap(QStringLiteral("video-display"), 64));
  ui->btnBrowseConfigFile->setIcon(deskflow::gui::themedIcon(QStringLiteral("document-open")));

  // force the first tab, since qt creator sets the active tab as the last one
  // the developer was looking at, and it's easy to accidentally save that.
  ui->tabWidget->setCurrentIndex(0);

  if (!deskflow::platform::isWindows())
    ui->cbWin32KeepForeground->setVisible(false);
  initConnections();
  onChange();
}

ServerConfigDialog::~ServerConfigDialog() = default;

bool ServerConfigDialog::addClient(const QString &clientName)
{
  return addComputer(clientName, true);
}

void ServerConfigDialog::accept()
{
  if (ui->groupExternalConfig->isChecked() && !QFile::exists(ui->lineConfigFile->text())) {

    auto selectedButton = QMessageBox::warning(
        this, "Filename invalid", "Please select a valid configuration file.", QMessageBox::Ok | QMessageBox::Ignore
    );

    if (selectedButton != QMessageBox::Ok || !browseConfigFile()) {
      return;
    }
  }

  // now that the dialog has been accepted, copy the new server config to the
  // original one, which is a reference to the one in MainWindow.
  setOriginalServerConfig(serverConfig());
  Settings::setValue(Settings::Server::Protocol, networkProtocolToOption(m_protocol));
  Settings::setValue(Settings::Server::EnableClipboard, m_enableClipboard);
  Settings::setValue(Settings::Server::ClipboardSize, m_clipboardSize);
  Settings::setValue(Settings::Server::EnableHeatbeat, m_enableHeartbeat);
  Settings::setValue(Settings::Server::Heartbeat, m_heartbeatRate);
  Settings::setValue(Settings::Server::EnableSwitchDelay, m_enableSwitchDelay);
  Settings::setValue(Settings::Server::SwitchDelay, m_switchDelay);
  Settings::setValue(Settings::Server::DefaultLockToComputerState, m_defaultLockToComputerState);
  Settings::setValue(Settings::Server::DisableLockToComputer, m_disableLockToComputer);
  Settings::setValue(Settings::Server::EnableSwitchDoubleTap, m_enableSwitchDoubleTap);
  Settings::setValue(Settings::Server::SwitchDoubleTap, m_switchDoubleTap);
  Settings::setValue(Settings::Server::RelativeMouseMoves, m_relativeMouseMoves);
  Settings::setValue(Settings::Server::Win32KeepForeground, m_win32keepForeground);

  const bool hidPassthrough = ui->groupHidPassthrough->isChecked();
  const bool gestureShare = ui->groupGestureSharing->isChecked();
  const auto sharingSecret = ui->lineGestureSecret->text();
  const auto hidDevices = ui->lineHidPassthroughDevices->text().trimmed();

  if (hidPassthrough && hidDevices.isEmpty()) {
    QMessageBox::warning(
        this, tr("HID passthrough"),
        tr("Enter at least one device selector (for example %1) before enabling HID passthrough.")
            .arg(QString::fromUtf8(deskflow::sharing::kHidDevicesPlaceholder))
    );
    return;
  }

  // Fork note: the server Sharing tab also writes client/* keys so single-machine
  // server+client roles stay in sync. Split-role deployments should configure
  // client settings separately on each machine.
  Settings::setValue(Settings::Server::HidPassthroughEnabled, hidPassthrough);
  Settings::setValue(Settings::Server::HidPassthroughDevices, hidDevices);
  Settings::setValue(Settings::Server::MouserBridgeEnabled, gestureShare || hidPassthrough);
  Settings::setValue(Settings::Client::MouserEnabled, hidPassthrough || gestureShare);
  Settings::setValue(Settings::Server::MouserBridgeToken, sharingSecret);
  Settings::setValue(Settings::Client::MouserToken, sharingSecret);

  QStringList screenNames;
  const auto screenList = m_screenSetupModel.m_Screens;
  for (const auto &screen : screenList) {
    const auto &screenName = screen.name();
    if (screenName.isEmpty())
      continue;
    screenNames.append(QStringLiteral("screen_%1").arg(screenName));
    Settings::setValue(Settings::Screen::Aliases.arg(screenName), screen.aliases());
  }
  Settings::removeUnknownScreens(screenNames);
  QDialog::accept();
}

void ServerConfigDialog::reject()
{
  serverConfig().setUseExternalConfig(m_originalServerConfigIsExternal);
  serverConfig().setConfigFile(m_originalServerConfigUsesExternalFile);

  QDialog::reject();
}

void ServerConfigDialog::addHotkey()
{
  Hotkey hotkey;
  HotkeyDialog dlg(this, hotkey);
  if (dlg.exec() == QDialog::Accepted) {
    serverConfig().hotkeys().append(hotkey);
    ui->listHotkeys->addItem(hotkey.text());
    onChange();
  }
}

void ServerConfigDialog::editHotkey()
{
  int row = ui->listHotkeys->currentRow();
  if (row < 0 || row >= serverConfig().hotkeys().size()) {
    qDebug() << "Attempt to editing out of bounds hotkey row: " << row;
    return;
  }

  Hotkey &hotkey = serverConfig().hotkeys()[row];
  HotkeyDialog dlg(this, hotkey);
  if (dlg.exec() == QDialog::Accepted) {
    ui->listHotkeys->currentItem()->setText(hotkey.text());
    onChange();
  }
}

void ServerConfigDialog::removeHotkey()
{
  int row = ui->listHotkeys->currentRow();
  if (row < 0 || row >= serverConfig().hotkeys().size()) {
    qDebug() << "Attempt to remove out of bounds hotkey row: " << row;
    return;
  }

  serverConfig().hotkeys().removeAt(row);
  ui->listActions->clear();
  delete ui->listHotkeys->item(row);
  onChange();
}

void ServerConfigDialog::listHotkeysSelectionChanged(const QItemSelection &selected, const QItemSelection &)
{
  bool itemsSelected = !selected.isEmpty();
  ui->btnEditHotkey->setEnabled(itemsSelected);
  ui->btnRemoveHotkey->setEnabled(itemsSelected);
  ui->btnNewAction->setEnabled(itemsSelected);

  if (itemsSelected && !serverConfig().hotkeys().isEmpty()) {
    ui->listActions->clear();
    const Hotkey &hotkey = serverConfig().hotkeys().at(selected.indexes().first().row());
    for (const Action &action : hotkey.actions())
      ui->listActions->addItem(action.text());
  }
}

void ServerConfigDialog::addAction()
{
  int row = ui->listHotkeys->currentRow();
  if (row < 0 || row >= serverConfig().hotkeys().size()) {
    qDebug() << "Attempt to add action to out of bounds hotkey row: " << row;
    return;
  }

  Hotkey &hotkey = serverConfig().hotkeys()[row];
  Action action;
  ActionDialog dlg(this, serverConfig(), hotkey, action);
  if (dlg.exec() == QDialog::Accepted) {
    hotkey.actions().append(action);
    ui->listActions->addItem(action.text());
    onChange();
  }
}

void ServerConfigDialog::editAction()
{
  int hotkeyRow = ui->listHotkeys->currentRow();
  if (hotkeyRow < 0 || hotkeyRow >= serverConfig().hotkeys().size()) {
    qDebug() << "Attempt to edit action from out of bounds hotkey row: " << hotkeyRow;
    return;
  }
  Hotkey &hotkey = serverConfig().hotkeys()[hotkeyRow];

  int actionRow = ui->listActions->currentRow();
  if (actionRow < 0 || actionRow >= hotkey.actions().size()) {
    qDebug() << "Attempt to remove out of bounds action row: " << actionRow;
    return;
  }
  Action &action = hotkey.actions()[actionRow];

  ActionDialog dlg(this, serverConfig(), hotkey, action);
  if (dlg.exec() == QDialog::Accepted) {
    ui->listActions->currentItem()->setText(action.text());
    onChange();
  }
}

void ServerConfigDialog::removeAction()
{
  int hotkeyRow = ui->listHotkeys->currentRow();
  if (hotkeyRow < 0 || hotkeyRow >= serverConfig().hotkeys().size()) {
    qDebug() << "Attempt to remove action from out of bounds hotkey row: " << hotkeyRow;
    return;
  }
  Hotkey &hotkey = serverConfig().hotkeys()[hotkeyRow];

  int actionRow = ui->listActions->currentRow();
  if (actionRow < 0 || actionRow >= hotkey.actions().size()) {
    qDebug() << "Attempt to remove out of bounds action row: " << actionRow;
    return;
  }

  hotkey.actions().removeAt(actionRow);
  delete ui->listActions->currentItem();
  onChange();
}

void ServerConfigDialog::toggleClipboard(bool enabled)
{
  if (m_enableClipboard == enabled)
    return;

  m_enableClipboard = enabled;

  ui->sbClipboardSizeLimit->setEnabled(enabled);
  if (enabled && !ui->sbClipboardSizeLimit->value()) {
    m_clipboardSize = Settings::defaultValue(Settings::Server::ClipboardSize).toUInt();
    ui->sbClipboardSizeLimit->setValue(m_clipboardSize ? m_clipboardSize : 1);
  }
  onChange();
}

void ServerConfigDialog::setClipboardLimit(int limit)
{
  if (m_clipboardSize == limit)
    return;

  m_clipboardSize = limit;
  onChange();
}

void ServerConfigDialog::toggleHeartbeat(bool enabled)
{
  m_enableHeartbeat = enabled;
  ui->sbHeartbeat->setEnabled(enabled);
  onChange();
}

void ServerConfigDialog::setHeartbeat(int rate)
{
  if (rate == m_heartbeatRate)
    return;
  m_heartbeatRate = rate;
  onChange();
}

void ServerConfigDialog::toggleRelativeMouseMoves(bool enabled)
{
  if (m_relativeMouseMoves == enabled)
    return;
  m_relativeMouseMoves = enabled;
  onChange();
}

void ServerConfigDialog::toggleProtocol()
{
  m_protocol = ui->rbProtocolBarrier->isChecked() ? NetworkProtocol::Barrier : NetworkProtocol::Synergy;
  onChange();
}

void ServerConfigDialog::listActionsSelectionChanged(const QItemSelection &selected, const QItemSelection &)
{
  bool enabled = !selected.isEmpty();
  ui->btnEditAction->setEnabled(enabled);
  ui->btnRemoveAction->setEnabled(enabled);
}

void ServerConfigDialog::toggleSwitchDoubleTap(bool enable)
{
  m_enableSwitchDoubleTap = enable;
  ui->sbSwitchDoubleTap->setEnabled(enable);
  onChange();
}

void ServerConfigDialog::setSwitchDoubleTap(int within)
{
  if (m_switchDoubleTap == within)
    return;
  m_switchDoubleTap = within;
  onChange();
}

void ServerConfigDialog::toggleSwitchDelay(bool enable)
{
  m_enableSwitchDelay = enable;
  ui->sbSwitchDelay->setEnabled(enable);
  onChange();
}

void ServerConfigDialog::setSwitchDelay(int delay)
{
  if (m_switchDelay == delay)
    return;
  m_switchDelay = delay;
  onChange();
}

void ServerConfigDialog::toggleDefaultLockToComputerState(bool state)
{
  if (m_defaultLockToComputerState == state)
    return;
  m_defaultLockToComputerState = state;
  onChange();
}

void ServerConfigDialog::toggleLockToComputer(bool disabled)
{
  if (m_disableLockToComputer == disabled)
    return;
  m_disableLockToComputer = disabled;
  onChange();
}

void ServerConfigDialog::toggleWin32Foreground(bool enabled)
{
  if (m_win32keepForeground == enabled)
    return;
  m_win32keepForeground = enabled;
  onChange();
}

void ServerConfigDialog::addClient()
{
  addComputer("", false);
}

void ServerConfigDialog::onScreenRemoved()
{
  ui->lblNewScreen->setEnabled(true);
  onChange();
}

void ServerConfigDialog::toggleExternalConfig(bool checked)
{
  ui->widgetExternalConfigControls->setEnabled(checked);
  ui->tabWidget->setTabEnabled(0, !checked);
  ui->tabWidget->setTabEnabled(1, !checked);
  serverConfig().setUseExternalConfig(checked);
  onChange();
}

bool ServerConfigDialog::browseConfigFile()
{
  //: %1 is replaced with the application names
  //: (*.conf) and (*.*) should not be translated
  const auto deskflowConfigFilter = tr("%1 Configurations (*.conf);;All files (*.*)");

  QString fileName =
      QFileDialog::getOpenFileName(this, tr("Browse for a config file"), "", deskflowConfigFilter.arg(kAppName));

  if (!fileName.isEmpty()) {
    ui->lineConfigFile->setText(fileName);
    serverConfig().setConfigFile(ui->lineConfigFile->text());
    onChange();
    return true;
  }

  return false;
}

void ServerConfigDialog::loadFromConfig()
{
  m_protocol = Settings::networkProtocol();
  ui->rbProtocolSynergy->setChecked(m_protocol == NetworkProtocol::Synergy);
  ui->rbProtocolBarrier->setChecked(m_protocol == NetworkProtocol::Barrier);

  ui->lineConfigFile->setText(serverConfig().configFile());

  m_enableHeartbeat = Settings::value(Settings::Server::EnableHeatbeat).toBool();
  ui->cbHeartbeat->setChecked(m_enableHeartbeat);
  ui->sbHeartbeat->setEnabled(ui->cbHeartbeat->isChecked());

  m_heartbeatRate = Settings::value(Settings::Server::Heartbeat).toInt();
  ui->sbHeartbeat->setValue(m_heartbeatRate);

  m_relativeMouseMoves = Settings::value(Settings::Server::RelativeMouseMoves).toBool();
  ui->cbRelativeMouseMoves->setChecked(m_relativeMouseMoves);

  m_win32keepForeground = Settings::value(Settings::Server::Win32KeepForeground).toBool();
  ui->cbWin32KeepForeground->setChecked(m_win32keepForeground);

  m_enableSwitchDelay = Settings::value(Settings::Server::EnableSwitchDelay).toBool();
  ui->cbSwitchDelay->setChecked(m_enableSwitchDelay);
  ui->sbSwitchDelay->setEnabled(ui->cbSwitchDelay->isChecked());

  m_switchDelay = Settings::value(Settings::Server::SwitchDelay).toInt();
  ui->sbSwitchDelay->setValue(m_switchDelay);

  m_enableSwitchDoubleTap = Settings::value(Settings::Server::EnableSwitchDoubleTap).toBool();
  ui->cbSwitchDoubleTap->setChecked(m_enableSwitchDoubleTap);
  ui->sbSwitchDoubleTap->setEnabled(ui->cbSwitchDoubleTap->isChecked());

  m_switchDoubleTap = Settings::value(Settings::Server::SwitchDoubleTap).toInt();
  ui->sbSwitchDoubleTap->setValue(m_switchDoubleTap);

  ui->groupExternalConfig->setChecked(serverConfig().useExternalConfig());

  ui->widgetExternalConfigControls->setEnabled(ui->groupExternalConfig->isChecked());
  toggleExternalConfig(ui->groupExternalConfig->isChecked());

  m_defaultLockToComputerState = Settings::value(Settings::Server::DefaultLockToComputerState).toBool();
  ui->cbDefaultLockToComputerState->setChecked(m_defaultLockToComputerState);

  m_disableLockToComputer = Settings::value(Settings::Server::DisableLockToComputer).toBool();
  ui->cbDisableLockToComputer->setChecked(m_disableLockToComputer);

  m_enableClipboard = Settings::value(Settings::Server::EnableClipboard).toBool();
  ui->cbEnableClipboard->setChecked(m_enableClipboard);
  ui->sbClipboardSizeLimit->setEnabled(m_enableClipboard);

  m_clipboardSize = Settings::value(Settings::Server::ClipboardSize).toUInt();
  ui->sbClipboardSizeLimit->setValue(m_clipboardSize);

  ui->listHotkeys->clear();
  for (const Hotkey &hotkey : std::as_const(serverConfig().hotkeys()))
    ui->listHotkeys->addItem(hotkey.text());

  ui->screenSetupView->setModel(&m_screenSetupModel);

  for (auto &screen : serverConfig().screens()) {
    if (!screen.isNull()) {
      screen.refreshPixmap();
    }
  }

  auto &screens = serverConfig().screens();
  auto server = std::ranges::find_if(screens, [this](const Screen &screen) {
    return (screen.name() == serverConfig().getServerName());
  });

  if (server == screens.end()) {
    Screen serverScreen(serverConfig().getServerName());
    serverScreen.markAsServer();
    model().screen(m_columns / 2, m_rows / 2) = serverScreen;
  } else {
    server->markAsServer();
  }

  m_hidPassthroughEnabled = Settings::value(Settings::Server::HidPassthroughEnabled).toBool();
  m_gestureShareEnabled = Settings::value(Settings::Server::MouserBridgeEnabled).toBool();
  m_hidPassthroughDevices = Settings::value(Settings::Server::HidPassthroughDevices).toString();
  m_sharingSecret = Settings::value(Settings::Server::MouserBridgeToken).toString();
  ui->groupHidPassthrough->setChecked(m_hidPassthroughEnabled);
  ui->groupGestureSharing->setChecked(m_gestureShareEnabled && !m_hidPassthroughEnabled);
  ui->lineHidPassthroughDevices->setPlaceholderText(
      QString::fromUtf8(deskflow::sharing::kHidDevicesPlaceholder)
  );
  ui->lineHidPassthroughDevices->setText(m_hidPassthroughDevices);
  ui->lineGestureSecret->setText(m_sharingSecret);
  updateSharingControls();
}

void ServerConfigDialog::updateSharingControls()
{
  const bool writable = Settings::isWritable();
  const bool hidEnabled = ui->groupHidPassthrough->isChecked();
  const bool sharingEnabled = hidEnabled || ui->groupGestureSharing->isChecked();
  ui->groupHidPassthrough->setEnabled(writable);
  ui->groupGestureSharing->setVisible(!hidEnabled);
  ui->groupGestureSharing->setEnabled(writable && !hidEnabled);
  ui->lineHidPassthroughDevices->setEnabled(writable && hidEnabled);
  ui->lineGestureSecret->setEnabled(writable && sharingEnabled);
}

void ServerConfigDialog::onHidPassthroughToggled(bool enabled)
{
  if (enabled) {
    QSignalBlocker blocker(ui->groupGestureSharing);
    ui->groupGestureSharing->setChecked(false);
  }
  updateSharingControls();
  onChange();
}

void ServerConfigDialog::onGestureSharingToggled(bool enabled)
{
  if (enabled) {
    QSignalBlocker blocker(ui->groupHidPassthrough);
    ui->groupHidPassthrough->setChecked(false);
  }
  updateSharingControls();
  onChange();
}

bool ServerConfigDialog::isSharingModified() const
{
  const auto hidDevices = ui->lineHidPassthroughDevices->text().trimmed();
  return ui->groupHidPassthrough->isChecked() != m_hidPassthroughEnabled ||
         ui->groupGestureSharing->isChecked() != m_gestureShareEnabled || hidDevices != m_hidPassthroughDevices ||
         ui->lineGestureSecret->text() != m_sharingSecret;
}

void ServerConfigDialog::initConnections() const
{
  connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ServerConfigDialog::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ServerConfigDialog::reject);
  connect(ui->lblRemoveScreen, &TrashScreenWidget::screenRemoved, this, &ServerConfigDialog::onScreenRemoved);
  connect(ui->btnNewHotkey, &QPushButton::clicked, this, &ServerConfigDialog::addHotkey);
  connect(ui->btnEditHotkey, &QPushButton::clicked, this, &ServerConfigDialog::editHotkey);
  connect(ui->btnRemoveHotkey, &QPushButton::clicked, this, &ServerConfigDialog::removeHotkey);
  connect(ui->listHotkeys, &QListView::doubleClicked, this, &ServerConfigDialog::editHotkey);
  connect(
      ui->listHotkeys->selectionModel(), &QItemSelectionModel::selectionChanged, this,
      &ServerConfigDialog::listHotkeysSelectionChanged
  );

  connect(ui->btnNewAction, &QPushButton::clicked, this, &ServerConfigDialog::addAction);
  connect(ui->btnEditAction, &QPushButton::clicked, this, &ServerConfigDialog::editAction);
  connect(ui->btnRemoveAction, &QPushButton::clicked, this, &ServerConfigDialog::removeAction);
  connect(ui->listActions, &QListView::doubleClicked, this, &ServerConfigDialog::editAction);
  connect(
      ui->listActions->selectionModel(), &QItemSelectionModel::selectionChanged, this,
      &ServerConfigDialog::listActionsSelectionChanged
  );

  connect(ui->rbProtocolBarrier, &QRadioButton::toggled, this, &ServerConfigDialog::toggleProtocol);
  connect(ui->cbHeartbeat, &QCheckBox::toggled, this, &ServerConfigDialog::toggleHeartbeat);
  connect(ui->sbHeartbeat, QOverload<int>::of(&QSpinBox::valueChanged), this, &ServerConfigDialog::setHeartbeat);
  connect(ui->cbWin32KeepForeground, &QCheckBox::toggled, this, &ServerConfigDialog::toggleWin32Foreground);
  connect(ui->cbSwitchDelay, &QCheckBox::toggled, this, &ServerConfigDialog::toggleSwitchDelay);
  connect(ui->sbSwitchDelay, QOverload<int>::of(&QSpinBox::valueChanged), this, &ServerConfigDialog::setSwitchDelay);
  connect(ui->cbSwitchDoubleTap, &QCheckBox::toggled, this, &ServerConfigDialog::toggleSwitchDoubleTap);
  connect(
      ui->sbSwitchDoubleTap, QOverload<int>::of(&QSpinBox::valueChanged), this, &ServerConfigDialog::setSwitchDoubleTap
  );

  connect(ui->cbRelativeMouseMoves, &QCheckBox::toggled, this, &ServerConfigDialog::toggleRelativeMouseMoves);
  connect(ui->cbEnableClipboard, &QCheckBox::toggled, this, &ServerConfigDialog::toggleClipboard);
  connect(ui->btnBrowseConfigFile, &QPushButton::clicked, this, &ServerConfigDialog::browseConfigFile);
  connect(ui->groupExternalConfig, &QGroupBox::toggled, this, &ServerConfigDialog::toggleExternalConfig);

  connect(
      ui->sbClipboardSizeLimit, QOverload<int>::of(&QSpinBox::valueChanged), this,
      &ServerConfigDialog::setClipboardLimit
  );
  connect(
      ui->cbDefaultLockToComputerState, &QCheckBox::toggled, this, &ServerConfigDialog::toggleDefaultLockToComputerState
  );
  connect(ui->cbDisableLockToComputer, &QCheckBox::toggled, this, &ServerConfigDialog::toggleLockToComputer);
  connect(&m_screenSetupModel, &ScreenSetupModel::screensChanged, this, &ServerConfigDialog::onChange);
  connect(ui->groupHidPassthrough, &QGroupBox::toggled, this, &ServerConfigDialog::onHidPassthroughToggled);
  connect(ui->groupGestureSharing, &QGroupBox::toggled, this, &ServerConfigDialog::onGestureSharingToggled);
  connect(ui->lineHidPassthroughDevices, &QLineEdit::textChanged, this, &ServerConfigDialog::onChange);
  connect(ui->lineGestureSecret, &QLineEdit::textChanged, this, &ServerConfigDialog::onChange);
}

bool ServerConfigDialog::addComputer(const QString &clientName, bool doSilent)
{
  bool isAccepted = false;
  Screen newScreen(clientName);

  if (ScreenSettingsDialog dlg(this, &newScreen, &model().m_Screens); doSilent || dlg.exec() == QDialog::Accepted) {
    model().addScreen(newScreen);
    isAccepted = true;
  }

  ui->lblNewScreen->setEnabled(!model().isFull());
  return isAccepted;
}

void ServerConfigDialog::onChange()
{
  bool isAppConfigDataEqual =
      m_originalServerConfigIsExternal == serverConfig().useExternalConfig() &&
      m_originalServerConfigUsesExternalFile == serverConfig().configFile() &&
      m_protocol == Settings::networkProtocol() &&
      m_enableClipboard == Settings::value(Settings::Server::EnableClipboard).toBool() &&
      m_clipboardSize == Settings::value(Settings::Server::ClipboardSize).toUInt() &&
      m_enableHeartbeat == Settings::value(Settings::Server::EnableHeatbeat).toBool() &&
      m_heartbeatRate == Settings::value(Settings::Server::Heartbeat).toInt() &&
      m_enableSwitchDelay == Settings::value(Settings::Server::EnableSwitchDelay).toBool() &&
      m_switchDelay == Settings::value(Settings::Server::SwitchDelay).toInt() &&
      m_enableSwitchDoubleTap == Settings::value(Settings::Server::EnableSwitchDoubleTap).toBool() &&
      m_switchDoubleTap == Settings::value(Settings::Server::SwitchDoubleTap).toInt() &&
      m_relativeMouseMoves == Settings::value(Settings::Server::RelativeMouseMoves).toBool() &&
      m_win32keepForeground == Settings::value(Settings::Server::Win32KeepForeground).toBool() &&
      m_disableLockToComputer == Settings::value(Settings::Server::DisableLockToComputer).toBool() &&
      m_defaultLockToComputerState == Settings::value(Settings::Server::DefaultLockToComputerState).toBool();
  ui->buttonBox->button(QDialogButtonBox::Ok)
      ->setEnabled(!isAppConfigDataEqual || !(m_originalServerConfig == m_serverConfig) || isSharingModified());
}
