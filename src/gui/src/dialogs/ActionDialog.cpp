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

#include "ActionDialog.h"
#include "ui_ActionDialog.h"

#include "Action.h"
#include "Hotkey.h"
#include "KeySequence.h"
#include "ServerConfig.h"

#include <QButtonGroup>

ActionDialog::ActionDialog(QWidget *parent, ServerConfig &config, Hotkey &hotkey, Action &action)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
      ui{std::make_unique<Ui::ActionDialog>()},
      m_ServerConfig(config),
      m_Hotkey(hotkey),
      m_Action(action),
      m_pButtonGroupType(new QButtonGroup(this))
{
  ui->setupUi(this);

  // work around Qt Designer's lack of a QButtonGroup; we need it to get
  // at the button id of the checked radio button
  QRadioButton *const typeButtons[] = {
      ui->m_pRadioPress,
      ui->m_pRadioRelease,
      ui->m_pRadioPressAndRelease,
      ui->m_pRadioSwitchToScreen,
      ui->m_pRadioSwitchInDirection,
      ui->m_pRadioLockCursorToScreen,
      ui->m_pRadioRestartAllConnections
  };

  for (unsigned int i = 0; i < sizeof(typeButtons) / sizeof(typeButtons[0]); i++)
    m_pButtonGroupType->addButton(typeButtons[i], i);

  ui->m_pKeySequenceWidgetHotkey->setText(m_Action.keySequence().toString());
  ui->m_pKeySequenceWidgetHotkey->setKeySequence(m_Action.keySequence());
  m_pButtonGroupType->button(m_Action.type())->setChecked(true);
  ui->m_pComboSwitchInDirection->setCurrentIndex(m_Action.switchDirection());
  ui->m_pComboLockCursorToScreen->setCurrentIndex(m_Action.lockCursorMode());

  if (m_Action.activeOnRelease())
    ui->m_pRadioHotkeyReleased->setChecked(true);
  else
    ui->m_pRadioHotkeyPressed->setChecked(true);

  ui->m_pGroupBoxScreens->setChecked(m_Action.haveScreens());

  int idx = 0;
  for (const Screen &screen : serverConfig().screens()) {
    if (!screen.isNull()) {
      QListWidgetItem *pListItem = new QListWidgetItem(screen.name());
      ui->m_pListScreens->addItem(pListItem);
      if (m_Action.typeScreenNames().indexOf(screen.name()) != -1)
        ui->m_pListScreens->setCurrentItem(pListItem);

      ui->m_pComboSwitchToScreen->addItem(screen.name());
      if (screen.name() == m_Action.switchScreenName())
        ui->m_pComboSwitchToScreen->setCurrentIndex(idx);

      idx++;
    }
  }
}

void ActionDialog::accept()
{
  if (!sequenceWidget()->valid() && m_pButtonGroupType->checkedId() >= 0 && m_pButtonGroupType->checkedId() < 3)
    return;

  m_Action.setKeySequence(sequenceWidget()->keySequence());
  m_Action.setType(m_pButtonGroupType->checkedId());
  m_Action.setHaveScreens(ui->m_pGroupBoxScreens->isChecked());

  m_Action.typeScreenNames().clear();

  const auto &selection = ui->m_pListScreens->selectedItems();
  for (const QListWidgetItem *pItem : selection)
    m_Action.typeScreenNames().append(pItem->text());

  m_Action.setSwitchScreenName(ui->m_pComboSwitchToScreen->currentText());
  m_Action.setSwitchDirection(ui->m_pComboSwitchInDirection->currentIndex());
  m_Action.setLockCursorMode(ui->m_pComboLockCursorToScreen->currentIndex());
  m_Action.setActiveOnRelease(ui->m_pRadioHotkeyReleased->isChecked());
  m_Action.setRestartServer(ui->m_pRadioRestartAllConnections->isChecked());

  QDialog::accept();
}

void ActionDialog::on_m_pKeySequenceWidgetHotkey_keySequenceChanged()
{
  if (sequenceWidget()->keySequence().isMouseButton()) {
    ui->m_pGroupBoxScreens->setEnabled(false);
    ui->m_pListScreens->setEnabled(false);
  } else {
    ui->m_pGroupBoxScreens->setEnabled(true);
    ui->m_pListScreens->setEnabled(true);
  }
}

const KeySequenceWidget *ActionDialog::sequenceWidget() const
{
  return ui->m_pKeySequenceWidgetHotkey;
}

ActionDialog::~ActionDialog() = default;
