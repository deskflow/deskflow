/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ServerConfigDialog.h"
#include "ui_ServerConfigDialog.h"

#include "common/constants.h"
#include "dialogs/ActionDialog.h"
#include "dialogs/HotkeyDialog.h"
#include "dialogs/ScreenSettingsDialog.h"

#include "ServerConfig.h"

#include <QFileDialog>
#include <QMessageBox>

using enum ScreenConfig::SwitchCorner;
using ServerProtocol = synergy::gui::ServerProtocol;

ServerConfigDialog::ServerConfigDialog(QWidget *parent, ServerConfig &config, AppConfig &appConfig)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
      ui{std::make_unique<Ui::ServerConfigDialog>()},
      m_OriginalServerConfig(config),
      m_OriginalServerConfigIsExternal(config.useExternalConfig()),
      m_OriginalServerConfigUsesExternalFile(config.configFile()),
      m_ServerConfig(config),
      m_ScreenSetupModel(serverConfig().screens(), serverConfig().numColumns(), serverConfig().numRows()),
      m_Message(""),
      m_appConfig(appConfig)
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

  ui->m_pButtonBrowseConfigFile->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::DocumentOpen));
  // force the first tab, since qt creator sets the active tab as the last one
  // the developer was looking at, and it's easy to accidentally save that.
  ui->tabWidget->setCurrentIndex(0);

  ui->m_pEditConfigFile->setText(serverConfig().configFile());
  ui->m_pCheckBoxUseExternalConfig->setChecked(serverConfig().useExternalConfig());
  ui->m_pCheckBoxHeartbeat->setChecked(serverConfig().hasHeartbeat());
  ui->m_pRadioProtocolSynergy->setChecked(serverConfig().protocol() == ServerProtocol::kSynergy);
  ui->m_pRadioProtocolBarrier->setChecked(serverConfig().protocol() == ServerProtocol::kBarrier);
  ui->m_pSpinBoxHeartbeat->setValue(serverConfig().heartbeat());

  ui->m_pCheckBoxRelativeMouseMoves->setChecked(serverConfig().relativeMouseMoves());
  ui->m_pCheckBoxWin32KeepForeground->setChecked(serverConfig().win32KeepForeground());

  ui->m_pCheckBoxSwitchDelay->setChecked(serverConfig().hasSwitchDelay());
  ui->m_pSpinBoxSwitchDelay->setValue(serverConfig().switchDelay());

  ui->m_pCheckBoxSwitchDoubleTap->setChecked(serverConfig().hasSwitchDoubleTap());
  ui->m_pSpinBoxSwitchDoubleTap->setValue(serverConfig().switchDoubleTap());

  ui->m_pCheckBoxCornerTopLeft->setChecked(serverConfig().switchCorner(static_cast<int>(TopLeft)));
  ui->m_pCheckBoxCornerTopRight->setChecked(serverConfig().switchCorner(static_cast<int>(TopRight)));
  ui->m_pCheckBoxCornerBottomLeft->setChecked(serverConfig().switchCorner(static_cast<int>(BottomLeft)));
  ui->m_pCheckBoxCornerBottomRight->setChecked(serverConfig().switchCorner(static_cast<int>(BottomRight)));
  ui->m_pSpinBoxSwitchCornerSize->setValue(serverConfig().switchCornerSize());
  ui->m_pCheckBoxDisableLockToScreen->setChecked(serverConfig().disableLockToScreen());

  ui->m_pCheckBoxEnableClipboard->setChecked(serverConfig().clipboardSharing());
  int clipboardSharingSizeM = static_cast<int>(serverConfig().clipboardSharingSize() / 1024);
  ui->m_pSpinBoxClipboardSizeLimit->setValue(clipboardSharingSizeM);
  ui->m_pSpinBoxClipboardSizeLimit->setEnabled(serverConfig().clipboardSharing());

  for (const Hotkey &hotkey : std::as_const(serverConfig().hotkeys()))
    ui->listHotkeys->addItem(hotkey.text());

  ui->screenSetupView->setModel(&m_ScreenSetupModel);

  auto &screens = serverConfig().screens();
  auto server = std::find_if(screens.begin(), screens.end(), [this](const Screen &screen) {
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
  connect(&m_ScreenSetupModel, &ScreenSetupModel::screensChanged, this, &ServerConfigDialog::onChange);

// Above Qt 6.7 the checkbox signal signature has changed from int to Qt::CheckState
#if QT_VERSION <= QT_VERSION_CHECK(6, 7, 0)
  // advanced
  connect(ui->m_pCheckBoxSwitchDelay, &QCheckBox::stateChanged, this, [this](const int &v) {
    serverConfig().haveSwitchDelay(v);
    onChange();
  });
  connect(ui->m_pCheckBoxSwitchDoubleTap, &QCheckBox::stateChanged, this, [this](const int &v) {
    serverConfig().haveSwitchDoubleTap(v);
    onChange();
  });
  connect(ui->m_pCheckBoxEnableClipboard, &QCheckBox::stateChanged, this, [this](const int &v) {
    serverConfig().setClipboardSharing(v);
    onChange();
  });
  connect(ui->m_pCheckBoxHeartbeat, &QCheckBox::stateChanged, this, [this](const int &v) {
    serverConfig().haveHeartbeat(v);
    onChange();
  });
  connect(ui->m_pCheckBoxRelativeMouseMoves, &QCheckBox::stateChanged, this, [this](const int &v) {
    serverConfig().setRelativeMouseMoves(v);
    onChange();
  });
  connect(ui->m_pCheckBoxWin32KeepForeground, &QCheckBox::stateChanged, this, [this](const int &v) {
    serverConfig().setWin32KeepForeground(v);
    onChange();
  });
  connect(ui->m_pCheckBoxDisableLockToScreen, &QCheckBox::stateChanged, this, [this](const int &v) {
    serverConfig().setDisableLockToScreen(v);
    onChange();
  });
  connect(ui->m_pCheckBoxCornerTopLeft, &QCheckBox::stateChanged, this, [this](const int &v) {
    serverConfig().setSwitchCorner(static_cast<int>(TopLeft), v);
    onChange();
  });
  connect(ui->m_pCheckBoxCornerTopRight, &QCheckBox::stateChanged, this, [this](const int &v) {
    serverConfig().setSwitchCorner(static_cast<int>(TopRight), v);
    onChange();
  });
  connect(ui->m_pCheckBoxCornerBottomLeft, &QCheckBox::stateChanged, this, [this](const int &v) {
    serverConfig().setSwitchCorner(static_cast<int>(BottomLeft), v);
    onChange();
  });
  connect(ui->m_pCheckBoxCornerBottomRight, &QCheckBox::stateChanged, this, [this](const int &v) {
    serverConfig().setSwitchCorner(static_cast<int>(BottomRight), v);
    onChange();
  });
  // config
  connect(ui->m_pCheckBoxUseExternalConfig, &QCheckBox::stateChanged, this, [this](const int &v) {
    serverConfig().setUseExternalConfig(v);
    onChange();
  });
#else
  connect(ui->m_pCheckBoxSwitchDelay, &QCheckBox::checkStateChanged, this, [this](const Qt::CheckState &v) {
    serverConfig().haveSwitchDelay(v == Qt::Checked);
    onChange();
  });
  connect(ui->m_pCheckBoxSwitchDoubleTap, &QCheckBox::checkStateChanged, this, [this](const Qt::CheckState &v) {
    serverConfig().haveSwitchDoubleTap(v == Qt::Checked);
    onChange();
  });
  connect(ui->m_pCheckBoxEnableClipboard, &QCheckBox::checkStateChanged, this, [this](const Qt::CheckState &v) {
    serverConfig().setClipboardSharing(v == Qt::Checked);
    onChange();
  });
  connect(ui->m_pCheckBoxHeartbeat, &QCheckBox::checkStateChanged, this, [this](const Qt::CheckState &v) {
    serverConfig().haveHeartbeat(v == Qt::Checked);
    onChange();
  });
  connect(ui->m_pCheckBoxRelativeMouseMoves, &QCheckBox::checkStateChanged, this, [this](const Qt::CheckState &v) {
    serverConfig().setRelativeMouseMoves(v == Qt::Checked);
    onChange();
  });
  connect(ui->m_pCheckBoxWin32KeepForeground, &QCheckBox::checkStateChanged, this, [this](const Qt::CheckState &v) {
    serverConfig().setWin32KeepForeground(v == Qt::Checked);
    onChange();
  });
  connect(ui->m_pCheckBoxDisableLockToScreen, &QCheckBox::checkStateChanged, this, [this](const Qt::CheckState &v) {
    serverConfig().setDisableLockToScreen(v == Qt::Checked);
    onChange();
  });
  connect(ui->m_pCheckBoxCornerTopLeft, &QCheckBox::checkStateChanged, this, [this](const Qt::CheckState &v) {
    serverConfig().setSwitchCorner(static_cast<int>(TopLeft), v == Qt::Checked);
    onChange();
  });
  connect(ui->m_pCheckBoxCornerTopRight, &QCheckBox::checkStateChanged, this, [this](const Qt::CheckState &v) {
    serverConfig().setSwitchCorner(static_cast<int>(TopRight), v == Qt::Checked);
    onChange();
  });
  connect(ui->m_pCheckBoxCornerBottomLeft, &QCheckBox::checkStateChanged, this, [this](const Qt::CheckState &v) {
    serverConfig().setSwitchCorner(static_cast<int>(BottomLeft), v == Qt::Checked);
    onChange();
  });
  connect(ui->m_pCheckBoxCornerBottomRight, &QCheckBox::checkStateChanged, this, [this](const Qt::CheckState &v) {
    serverConfig().setSwitchCorner(static_cast<int>(BottomRight), v == Qt::Checked);
    onChange();
  });
  // config
  connect(ui->m_pCheckBoxUseExternalConfig, &QCheckBox::checkStateChanged, this, [this](const Qt::CheckState &v) {
    serverConfig().setUseExternalConfig(v == Qt::Checked);
    onChange();
  });
#endif

  connect(
      ui->m_pSpinBoxSwitchDelay, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
      [this](const int &v) {
        serverConfig().setSwitchDelay(v);
        onChange();
      }
  );
  connect(
      ui->m_pSpinBoxSwitchDoubleTap, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
      [this](const int &v) {
        serverConfig().setSwitchDoubleTap(v);
        onChange();
      }
  );
  connect(
      ui->m_pSpinBoxClipboardSizeLimit, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
      [this](const int &v) {
        serverConfig().setClipboardSharingSize(v * 1024);
        onChange();
      }
  );
  connect(
      ui->m_pSpinBoxHeartbeat, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
      [this](const int &v) {
        serverConfig().setHeartbeat(v);
        onChange();
      }
  );
  connect(
      ui->m_pSpinBoxSwitchCornerSize, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
      [this](const int &v) {
        serverConfig().setSwitchCornerSize(v);
        onChange();
      }
  );
  connect(ui->m_pRadioProtocolSynergy, &QRadioButton::toggled, this, [this](const bool &v) {
    if (v) {
      serverConfig().setProtocol(ServerProtocol::kSynergy);
      onChange();
    }
  });
  connect(ui->m_pRadioProtocolBarrier, &QRadioButton::toggled, this, [this](const bool &v) {
    if (v) {
      serverConfig().setProtocol(ServerProtocol::kBarrier);
      onChange();
    }
  });

  connect(ui->m_pEditConfigFile, &QLineEdit::textChanged, this, [this]() {
    serverConfig().setConfigFile(ui->m_pEditConfigFile->text());
    onChange();
  });
}

ServerConfigDialog::~ServerConfigDialog() = default;

bool ServerConfigDialog::addClient(const QString &clientName)
{
  return addComputer(clientName, true);
}

void ServerConfigDialog::accept()
{
  if (ui->m_pCheckBoxUseExternalConfig->isChecked() && !QFile::exists(ui->m_pEditConfigFile->text())) {

    auto selectedButton = QMessageBox::warning(
        this, "Filename invalid", "Please select a valid configuration file.", QMessageBox::Ok | QMessageBox::Ignore
    );

    if (selectedButton != QMessageBox::Ok || !on_m_pButtonBrowseConfigFile_clicked()) {
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
  serverConfig().setUseExternalConfig(m_OriginalServerConfigIsExternal);
  serverConfig().setConfigFile(m_OriginalServerConfigUsesExternalFile);

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
    qDebug() << "Atempt to editing out of bounds hotkey row: " << row;
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
    qDebug() << "Atempt to remove out of bounds hotkey row: " << row;
    return;
  }

  serverConfig().hotkeys().removeAt(row);
  ui->listActions->clear();
  delete ui->listHotkeys->item(row);
  onChange();
}

void ServerConfigDialog::listHotkeysSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
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
    qDebug() << "Atempt to add action to out of bounds hotkey row: " << row;
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
    qDebug() << "Atempt to edit action from out of bounds hotkey row: " << hotkeyRow;
    return;
  }
  Hotkey &hotkey = serverConfig().hotkeys()[hotkeyRow];

  int actionRow = ui->listActions->currentRow();
  if (actionRow < 0 || actionRow >= hotkey.actions().size()) {
    qDebug() << "Atempt to remove out of bounds action row: " << actionRow;
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
    qDebug() << "Atempt to remove action from out of bounds hotkey row: " << hotkeyRow;
    return;
  }
  Hotkey &hotkey = serverConfig().hotkeys()[hotkeyRow];

  int actionRow = ui->listActions->currentRow();
  if (actionRow < 0 || actionRow >= hotkey.actions().size()) {
    qDebug() << "Atempt to remove out of bounds action row: " << actionRow;
    return;
  }

  hotkey.actions().removeAt(actionRow);
  delete ui->listActions->currentItem();
  onChange();
}

void ServerConfigDialog::on_m_pCheckBoxEnableClipboard_stateChanged(int const state)
{
  ui->m_pSpinBoxClipboardSizeLimit->setEnabled(state == Qt::Checked);
  if ((state == Qt::Checked) && (!ui->m_pSpinBoxClipboardSizeLimit->value())) {
    int size = static_cast<int>((serverConfig().defaultClipboardSharingSize() + 512) / 1024);
    ui->m_pSpinBoxClipboardSizeLimit->setValue(size ? size : 1);
  }
}

void ServerConfigDialog::listActionsSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
  bool enabled = !selected.isEmpty();
  ui->btnEditAction->setEnabled(enabled);
  ui->btnRemoveAction->setEnabled(enabled);
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

void ServerConfigDialog::on_m_pCheckBoxUseExternalConfig_toggled(bool checked)
{
  ui->m_pLabelConfigFile->setEnabled(checked);
  ui->m_pEditConfigFile->setEnabled(checked);
  ui->m_pButtonBrowseConfigFile->setEnabled(checked);

  ui->tabWidget->setTabEnabled(0, !checked);
  ui->tabWidget->setTabEnabled(1, !checked);
  ui->tabWidget->setTabEnabled(2, !checked);
}

bool ServerConfigDialog::on_m_pButtonBrowseConfigFile_clicked()
{
#if defined(Q_OS_WIN)
  static const auto configExt = QStringLiteral("sgc");
#else
  static const auto configExt = QStringLiteral("conf");
#endif
  static const auto deskflowConfigFilter = QStringLiteral("%1 Configurations (*.%2);;All files (*.*)");

  QString fileName =
      QFileDialog::getOpenFileName(this, "Browse for a config file", "", deskflowConfigFilter.arg(kAppName, configExt));

  if (!fileName.isEmpty()) {
    ui->m_pEditConfigFile->setText(fileName);
    return true;
  }

  return false;
}

bool ServerConfigDialog::addComputer(const QString &clientName, bool doSilent)
{
  bool isAccepted = false;
  Screen newScreen(clientName);

  ScreenSettingsDialog dlg(this, &newScreen, &model().m_Screens);
  if (doSilent || dlg.exec() == QDialog::Accepted) {
    model().addScreen(newScreen);
    isAccepted = true;
  }

  ui->lblNewScreen->setEnabled(!model().isFull());
  return isAccepted;
}

void ServerConfigDialog::onChange()
{
  bool isAppConfigDataEqual = m_OriginalServerConfigIsExternal == serverConfig().useExternalConfig() &&
                              m_OriginalServerConfigUsesExternalFile == serverConfig().configFile();
  ui->buttonBox->button(QDialogButtonBox::Ok)
      ->setEnabled(!isAppConfigDataEqual || !(m_OriginalServerConfig == m_ServerConfig));
}
