/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ServerConfigDialog.h"
#include "common/PlatformInfo.h"
#include "ui_ServerConfigDialog.h"

#include "base/NetworkProtocol.h"
#include "common/Constants.h"
#include "dialogs/ActionDialog.h"
#include "dialogs/HotkeyDialog.h"
#include "dialogs/ScreenSettingsDialog.h"

#include <QFileDialog>
#include <QMessageBox>

using enum ScreenConfig::SwitchCorner;
using enum NetworkProtocol;

ServerConfigDialog::ServerConfigDialog(QWidget *parent, ServerConfig &config)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
      ui{std::make_unique<Ui::ServerConfigDialog>()},
      m_originalServerConfig(config),
      m_originalServerConfigIsExternal(config.useExternalConfig()),
      m_originalServerConfigUsesExternalFile(config.configFile()),
      m_serverConfig(config),
      m_screenSetupModel(m_serverConfig.screens(), m_serverConfig.numColumns(), m_serverConfig.numRows())
{
  ui->setupUi(this);

  connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ServerConfigDialog::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ServerConfigDialog::reject);

  ui->lblRemoveScreen->setPixmap(QIcon::fromTheme("user-trash").pixmap(QSize(64, 64)));
  connect(ui->lblRemoveScreen, &TrashScreenWidget::screenRemoved, this, &ServerConfigDialog::onScreenRemoved);

  ui->lblNewScreen->setEnabled(!model().isFull());
  ui->lblNewScreen->setPixmap(QIcon::fromTheme("video-display").pixmap(QSize(64, 64)));

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

  // force the first tab, since qt creator sets the active tab as the last one
  // the developer was looking at, and it's easy to accidentally save that.
  ui->tabWidget->setCurrentIndex(0);

  ui->btnBrowseConfigFile->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::DocumentOpen));
  ui->lineConfigFile->setText(serverConfig().configFile());

  ui->rbProtocolSynergy->setChecked(serverConfig().protocol() == NetworkProtocol::Synergy);
  ui->rbProtocolBarrier->setChecked(serverConfig().protocol() == NetworkProtocol::Barrier);
  connect(ui->rbProtocolBarrier, &QRadioButton::toggled, this, &ServerConfigDialog::toggleProtocol);

  ui->cbHeartbeat->setChecked(serverConfig().hasHeartbeat());
  connect(ui->cbHeartbeat, &QCheckBox::toggled, this, &ServerConfigDialog::toggleHeartbeat);

  ui->sbHeartbeat->setEnabled(ui->cbHeartbeat->isChecked());
  ui->sbHeartbeat->setValue(serverConfig().heartbeat());
  connect(ui->sbHeartbeat, QOverload<int>::of(&QSpinBox::valueChanged), this, &ServerConfigDialog::setHeartbeat);

  ui->cbRelativeMouseMoves->setChecked(serverConfig().relativeMouseMoves());

  if (!deskflow::platform::isWindows())
    ui->cbWin32KeepForeground->setVisible(false);

  ui->cbWin32KeepForeground->setChecked(serverConfig().win32KeepForeground());
  connect(ui->cbWin32KeepForeground, &QCheckBox::toggled, this, &ServerConfigDialog::toggleWin32Foreground);

  ui->cbSwitchDelay->setChecked(serverConfig().hasSwitchDelay());
  connect(ui->cbSwitchDelay, &QCheckBox::toggled, this, &ServerConfigDialog::toggleSwitchDelay);

  ui->sbSwitchDelay->setEnabled(ui->cbSwitchDelay->isChecked());
  ui->sbSwitchDelay->setValue(serverConfig().switchDelay());
  connect(ui->sbSwitchDelay, QOverload<int>::of(&QSpinBox::valueChanged), this, &ServerConfigDialog::setSwitchDelay);

  ui->cbSwitchDoubleTap->setChecked(serverConfig().hasSwitchDoubleTap());
  connect(ui->cbSwitchDoubleTap, &QCheckBox::toggled, this, &ServerConfigDialog::toggleSwitchDoubleTap);

  ui->sbSwitchDoubleTap->setEnabled(ui->cbSwitchDoubleTap->isChecked());
  ui->sbSwitchDoubleTap->setValue(serverConfig().switchDoubleTap());
  connect(
      ui->sbSwitchDoubleTap, QOverload<int>::of(&QSpinBox::valueChanged), this, &ServerConfigDialog::setSwitchDoubleTap
  );

  connect(ui->cbRelativeMouseMoves, &QCheckBox::toggled, this, &ServerConfigDialog::toggleRelativeMouseMoves);
  connect(ui->cbEnableClipboard, &QCheckBox::toggled, this, &ServerConfigDialog::toggleClipboard);

  connect(ui->btnBrowseConfigFile, &QPushButton::clicked, this, &ServerConfigDialog::browseConfigFile);

  ui->groupExternalConfig->setChecked(serverConfig().useExternalConfig());
  ui->widgetExternalConfigControls->setEnabled(ui->groupExternalConfig->isChecked());
  ui->tabWidget->setTabEnabled(0, !ui->groupExternalConfig->isChecked());
  ui->tabWidget->setTabEnabled(1, !ui->groupExternalConfig->isChecked());
  ui->tabWidget->setTabEnabled(2, !ui->groupExternalConfig->isChecked());
  connect(ui->groupExternalConfig, &QGroupBox::toggled, this, &ServerConfigDialog::toggleExternalConfig);

  connect(
      ui->sbSwitchCornerSize, QOverload<int>::of(&QSpinBox::valueChanged), this,
      &ServerConfigDialog::setSwitchCornerSize
  );
  connect(
      ui->sbClipboardSizeLimit, QOverload<int>::of(&QSpinBox::valueChanged), this,
      &ServerConfigDialog::setClipboardLimit
  );

  ui->cbCornerTopLeft->setChecked(serverConfig().switchCorner(static_cast<int>(TopLeft)));
  connect(ui->cbCornerTopLeft, &QCheckBox::toggled, this, &ServerConfigDialog::toggleCornerTopLeft);

  ui->cbCornerTopRight->setChecked(serverConfig().switchCorner(static_cast<int>(TopRight)));
  connect(ui->cbCornerTopRight, &QCheckBox::toggled, this, &ServerConfigDialog::toggleCornerTopRight);

  ui->cbCornerBottomLeft->setChecked(serverConfig().switchCorner(static_cast<int>(BottomLeft)));
  connect(ui->cbCornerBottomLeft, &QCheckBox::toggled, this, &ServerConfigDialog::toggleCornerBottomLeft);

  ui->cbCornerBottomRight->setChecked(serverConfig().switchCorner(static_cast<int>(BottomRight)));
  connect(ui->cbCornerBottomRight, &QCheckBox::toggled, this, &ServerConfigDialog::toggleCornerBottomRight);

  ui->sbSwitchCornerSize->setValue(serverConfig().switchCornerSize());

  ui->cbDisableLockToScreen->setChecked(serverConfig().disableLockToScreen());
  connect(ui->cbDisableLockToScreen, &QCheckBox::toggled, this, &ServerConfigDialog::toggleLockToScreen);

  ui->cbEnableClipboard->setChecked(serverConfig().clipboardSharing());
  auto clipboardSharingSizeM = static_cast<int>(serverConfig().clipboardSharingSize() / 1024);
  ui->sbClipboardSizeLimit->setValue(clipboardSharingSizeM);
  ui->sbClipboardSizeLimit->setEnabled(serverConfig().clipboardSharing());

  for (const Hotkey &hotkey : std::as_const(serverConfig().hotkeys()))
    ui->listHotkeys->addItem(hotkey.text());

  ui->screenSetupView->setModel(&m_screenSetupModel);

  auto &screens = serverConfig().screens();
  auto server = std::ranges::find_if(screens, [this](const Screen &screen) {
    return (screen.name() == serverConfig().getServerName());
  });

  if (server == screens.end()) {
    Screen serverScreen(serverConfig().getServerName());
    serverScreen.markAsServer();
    model().screen(serverConfig().numColumns() / 2, serverConfig().numRows() / 2) = serverScreen;
  } else {
    server->markAsServer();
  }

  onChange();

  // computers
  connect(&m_screenSetupModel, &ScreenSetupModel::screensChanged, this, &ServerConfigDialog::onChange);
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
  ui->sbClipboardSizeLimit->setEnabled(enabled);
  if (enabled && !ui->sbClipboardSizeLimit->value()) {
    auto size = static_cast<int>((ServerConfig::defaultClipboardSharingSize() + 512) / 1024);
    ui->sbClipboardSizeLimit->setValue(size ? size : 1);
  }
  serverConfig().setClipboardSharing(enabled);
  onChange();
}

void ServerConfigDialog::setClipboardLimit(int limit)
{
  serverConfig().setClipboardSharingSize(limit * 1024);
  onChange();
}

void ServerConfigDialog::toggleHeartbeat(bool enabled)
{
  ui->sbHeartbeat->setEnabled(enabled);
  serverConfig().haveHeartbeat(enabled);
  onChange();
}

void ServerConfigDialog::setHeartbeat(int rate)
{
  serverConfig().setHeartbeat(rate);
  onChange();
}

void ServerConfigDialog::toggleRelativeMouseMoves(bool enabled)
{
  serverConfig().setRelativeMouseMoves(enabled);
  onChange();
}

void ServerConfigDialog::toggleProtocol()
{
  auto proto = ui->rbProtocolBarrier->isChecked() ? NetworkProtocol::Barrier : NetworkProtocol::Synergy;
  serverConfig().setProtocol(proto);
  onChange();
}

void ServerConfigDialog::setSwitchCornerSize(int size)
{
  serverConfig().setSwitchCornerSize(size);
  onChange();
}

void ServerConfigDialog::toggleCornerBottomLeft(bool enable)
{
  serverConfig().setSwitchCorner(static_cast<int>(BottomLeft), enable);
  onChange();
}

void ServerConfigDialog::toggleCornerTopLeft(bool enable)
{
  serverConfig().setSwitchCorner(static_cast<int>(TopLeft), enable);
  onChange();
}

void ServerConfigDialog::toggleCornerBottomRight(bool enable)
{
  serverConfig().setSwitchCorner(static_cast<int>(BottomRight), enable);
  onChange();
}

void ServerConfigDialog::toggleCornerTopRight(bool enable)
{
  serverConfig().setSwitchCorner(static_cast<int>(TopRight), enable);
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
  ui->sbSwitchDoubleTap->setEnabled(enable);
  serverConfig().haveSwitchDoubleTap(enable);
  onChange();
}

void ServerConfigDialog::setSwitchDoubleTap(int within)
{
  serverConfig().setSwitchDoubleTap(within);
  onChange();
}

void ServerConfigDialog::toggleSwitchDelay(bool enable)
{
  ui->sbSwitchDelay->setEnabled(enable);
  serverConfig().haveSwitchDelay(enable);
  onChange();
}

void ServerConfigDialog::setSwitchDelay(int delay)
{
  serverConfig().setSwitchDelay(delay);
  onChange();
}

void ServerConfigDialog::toggleLockToScreen(bool disabled)
{
  serverConfig().setDisableLockToScreen(disabled);
  onChange();
}

void ServerConfigDialog::toggleWin32Foreground(bool enabled)
{
  serverConfig().setWin32KeepForeground(enabled);
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
  ui->tabWidget->setTabEnabled(2, !checked);

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
  bool isAppConfigDataEqual = m_originalServerConfigIsExternal == serverConfig().useExternalConfig() &&
                              m_originalServerConfigUsesExternalFile == serverConfig().configFile();
  ui->buttonBox->button(QDialogButtonBox::Ok)
      ->setEnabled(!isAppConfigDataEqual || !(m_originalServerConfig == m_serverConfig));
}
