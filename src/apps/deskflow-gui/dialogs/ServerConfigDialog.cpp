/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
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

  const auto folderIcon =
      QIcon::fromTheme(QIcon::ThemeIcon::DocumentOpen, QIcon(QStringLiteral(":/icons/64x64/folder.png")));
  ui->m_pButtonBrowseConfigFile->setIcon(folderIcon);

  // force the first tab, since qt creator sets the active tab as the last one
  // the developer was looking at, and it's easy to accidentally save that.
  ui->m_pTabWidget->setCurrentIndex(0);

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
    ui->m_pListHotkeys->addItem(hotkey.text());

  ui->m_pScreenSetupView->setModel(&m_ScreenSetupModel);

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

  ui->m_pButtonAddComputer->setEnabled(!model().isFull());
  connect(ui->m_pTrashScreenWidget, &TrashScreenWidget::screenRemoved, this, &ServerConfigDialog::onScreenRemoved);

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

void ServerConfigDialog::on_m_pButtonNewHotkey_clicked()
{
  Hotkey hotkey;
  HotkeyDialog dlg(this, hotkey);
  if (dlg.exec() == QDialog::Accepted) {
    serverConfig().hotkeys().append(hotkey);
    ui->m_pListHotkeys->addItem(hotkey.text());
    onChange();
  }
}

void ServerConfigDialog::on_m_pButtonEditHotkey_clicked()
{
  int idx = ui->m_pListHotkeys->currentRow();
  Q_ASSERT(idx >= 0 && idx < serverConfig().hotkeys().size());
  Hotkey &hotkey = serverConfig().hotkeys()[idx];
  HotkeyDialog dlg(this, hotkey);
  if (dlg.exec() == QDialog::Accepted) {
    ui->m_pListHotkeys->currentItem()->setText(hotkey.text());
    onChange();
  }
}

void ServerConfigDialog::on_m_pButtonRemoveHotkey_clicked()
{
  int idx = ui->m_pListHotkeys->currentRow();
  Q_ASSERT(idx >= 0 && idx < serverConfig().hotkeys().size());
  serverConfig().hotkeys().removeAt(idx);
  ui->m_pListActions->clear();
  delete ui->m_pListHotkeys->item(idx);
  onChange();
}

void ServerConfigDialog::on_m_pListHotkeys_itemSelectionChanged()
{
  bool itemsSelected = !ui->m_pListHotkeys->selectedItems().isEmpty();
  ui->m_pButtonEditHotkey->setEnabled(itemsSelected);
  ui->m_pButtonRemoveHotkey->setEnabled(itemsSelected);
  ui->m_pButtonNewAction->setEnabled(itemsSelected);

  if (itemsSelected && serverConfig().hotkeys().size() > 0) {
    ui->m_pListActions->clear();

    int idx = ui->m_pListHotkeys->row(ui->m_pListHotkeys->selectedItems()[0]);

    // There's a bug somewhere around here: We get idx == 1 right after we
    // deleted the next to last item, so idx can only possibly be 0. GDB shows
    // we got called indirectly from the delete line in
    // on_m_pButtonRemoveHotkey_clicked() above, but the delete is of course
    // necessary and seems correct. The while() is a generalized workaround for
    // all that and shouldn't be required.
    while (idx >= 0 && idx >= serverConfig().hotkeys().size())
      idx--;

    Q_ASSERT(idx >= 0 && idx < serverConfig().hotkeys().size());

    const Hotkey &hotkey = serverConfig().hotkeys()[idx];
    for (const Action &action : hotkey.actions())
      ui->m_pListActions->addItem(action.text());
  }
}

void ServerConfigDialog::on_m_pButtonNewAction_clicked()
{
  int idx = ui->m_pListHotkeys->currentRow();
  Q_ASSERT(idx >= 0 && idx < serverConfig().hotkeys().size());
  Hotkey &hotkey = serverConfig().hotkeys()[idx];

  Action action;
  ActionDialog dlg(this, serverConfig(), hotkey, action);
  if (dlg.exec() == QDialog::Accepted) {
    hotkey.actions().append(action);
    ui->m_pListActions->addItem(action.text());
    onChange();
  }
}

void ServerConfigDialog::on_m_pButtonEditAction_clicked()
{
  int idxHotkey = ui->m_pListHotkeys->currentRow();
  Q_ASSERT(idxHotkey >= 0 && idxHotkey < serverConfig().hotkeys().size());
  Hotkey &hotkey = serverConfig().hotkeys()[idxHotkey];

  int idxAction = ui->m_pListActions->currentRow();
  Q_ASSERT(idxAction >= 0 && idxAction < hotkey.actions().size());
  Action &action = hotkey.actions()[idxAction];

  ActionDialog dlg(this, serverConfig(), hotkey, action);
  if (dlg.exec() == QDialog::Accepted) {
    ui->m_pListActions->currentItem()->setText(action.text());
    onChange();
  }
}

void ServerConfigDialog::on_m_pButtonRemoveAction_clicked()
{
  int idxHotkey = ui->m_pListHotkeys->currentRow();
  Q_ASSERT(idxHotkey >= 0 && idxHotkey < serverConfig().hotkeys().size());
  Hotkey &hotkey = serverConfig().hotkeys()[idxHotkey];

  int idxAction = ui->m_pListActions->currentRow();
  Q_ASSERT(idxAction >= 0 && idxAction < hotkey.actions().size());

  hotkey.actions().removeAt(idxAction);
  delete ui->m_pListActions->currentItem();
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

void ServerConfigDialog::on_m_pListActions_itemSelectionChanged()
{
  ui->m_pButtonEditAction->setEnabled(!ui->m_pListActions->selectedItems().isEmpty());
  ui->m_pButtonRemoveAction->setEnabled(!ui->m_pListActions->selectedItems().isEmpty());
}

void ServerConfigDialog::on_m_pButtonAddComputer_clicked()
{
  addComputer("", false);
}

void ServerConfigDialog::onScreenRemoved()
{
  ui->m_pButtonAddComputer->setEnabled(true);
  onChange();
}

void ServerConfigDialog::on_m_pCheckBoxUseExternalConfig_toggled(bool checked)
{
  ui->m_pLabelConfigFile->setEnabled(checked);
  ui->m_pEditConfigFile->setEnabled(checked);
  ui->m_pButtonBrowseConfigFile->setEnabled(checked);

  ui->m_pTabWidget->setTabEnabled(0, !checked);
  ui->m_pTabWidget->setTabEnabled(1, !checked);
  ui->m_pTabWidget->setTabEnabled(2, !checked);
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

  ui->m_pButtonAddComputer->setEnabled(!model().isFull());
  return isAccepted;
}

void ServerConfigDialog::onChange()
{
  bool isAppConfigDataEqual = m_OriginalServerConfigIsExternal == serverConfig().useExternalConfig() &&
                              m_OriginalServerConfigUsesExternalFile == serverConfig().configFile();
  ui->m_pButtonBox->button(QDialogButtonBox::Ok)
      ->setEnabled(!isAppConfigDataEqual || !(m_OriginalServerConfig == m_ServerConfig));
}
