/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ActionDialog.h"
#include "ui_ActionDialog.h"

#include "common/Action.h"
#include "common/Hotkey.h"
#include "common/KeySequence.h"
#include "config/ServerConfig.h"

#include <QTimer>

ActionDialog::ActionDialog(QWidget *parent, const ServerConfig &config, Hotkey &hotkey, Action &action)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
      ui{std::make_unique<Ui::ActionDialog>()},
      m_hotkey(hotkey),
      m_action(action)
{
  ui->setupUi(this);
  connect(ui->keySequenceWidget, &KeySequenceWidget::keySequenceChanged, this, &ActionDialog::keySequenceChanged);
  connect(
      ui->comboActionType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ActionDialog::actionTypeChanged
  );
  connect(ui->listComputers, &QListWidget::itemChanged, this, &ActionDialog::itemToggled);
  connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ActionDialog::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ActionDialog::reject);

  ui->keySequenceWidget->setText(m_action.keySequence().toString());
  ui->keySequenceWidget->setKeySequence(m_action.keySequence());

  ui->comboSwitchInDirection->setCurrentIndex(m_action.switchDirection());
  ui->comboLockCursorToComputer->setCurrentIndex(m_action.lockCursorMode());

  ui->comboActionType->setCurrentIndex(m_action.type());
  ui->comboTriggerOn->setCurrentIndex(m_action.activeOnRelease());

  for (const Computer &computer : config.computers()) {
    if (computer.isNull())
      continue;
    auto *newListItem = new QListWidgetItem(computer.name());
    newListItem->setCheckState(Qt::Checked);
    if ((m_action.typeScreenNames().indexOf(computer.name()) == -1) &&
        (m_action.haveScreens() && !m_action.typeScreenNames().isEmpty()))
      newListItem->setCheckState(Qt::Unchecked);

    ui->listComputers->addItem(newListItem);

    ui->comboSwitchToComputer->addItem(tr("Switch to %1").arg(computer.name()), computer.name());
    if (computer.name() == m_action.switchScreenName())
      ui->comboSwitchToComputer->setCurrentIndex(ui->comboSwitchToComputer->count() - 1);
  }

  ui->keySequenceWidget->setVisible(false);
  ui->groupComputers->setVisible(false);
  ui->listComputers->setEnabled(!ui->keySequenceWidget->keySequence().isMouseButton());
  ui->comboSwitchToComputer->setVisible(false);
  ui->comboSwitchInDirection->setVisible(false);
  ui->comboLockCursorToComputer->setVisible(false);

  actionTypeChanged(ui->comboActionType->currentIndex());
}

void ActionDialog::accept()
{
  if (!canSave())
    return;

  m_action.setKeySequence(ui->keySequenceWidget->keySequence());
  m_action.setType(ui->comboActionType->currentIndex());

  m_action.clearScreens();

  int screenCount = ui->listComputers->count();

  for (int i = 0; i < ui->listComputers->count(); i++) {
    const auto &item = ui->listComputers->item(i);
    m_action.addComputer(item->text());
    if (item->checkState() == Qt::Unchecked) {
      screenCount--;
      m_action.removeScreen(item->text());
    }
  }

  if (screenCount == ui->listComputers->count())
    m_action.clearScreens();

  m_action.setHaveScreens(screenCount);

  m_action.setSwitchScreenName(ui->comboSwitchToComputer->currentData().toString());
  m_action.setSwitchDirection(ui->comboSwitchInDirection->currentIndex());
  m_action.setLockCursorMode(ui->comboLockCursorToComputer->currentIndex());
  m_action.setActiveOnRelease(ui->comboTriggerOn->currentIndex());
  m_action.setRestartServer(ui->comboActionType->currentIndex() == ActionTypes::RestartServer);

  QDialog::accept();
}

void ActionDialog::updateSize()
{
  setMaximumSize(QSize(16777215, 1677215));
  adjustSize();
  if (!isKeyAction(ui->comboActionType->currentIndex()))
    setMaximumSize(size());
}

void ActionDialog::keySequenceChanged()
{
  ui->listComputers->setEnabled(!ui->keySequenceWidget->keySequence().isMouseButton());
  ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(canSave());
}

void ActionDialog::itemToggled() const
{
  ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(canSave());
}

void ActionDialog::actionTypeChanged(int index)
{
  ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(canSave());
  ui->keySequenceWidget->setVisible(isKeyAction(index));
  ui->groupComputers->setVisible(isKeyAction(index));
  ui->listComputers->setEnabled(!ui->keySequenceWidget->keySequence().isMouseButton());
  ui->comboSwitchToComputer->setVisible(index == ActionTypes::SwitchTo);
  ui->comboSwitchInDirection->setVisible(index == ActionTypes::SwitchInDirection);
  ui->comboLockCursorToComputer->setVisible(index == ActionTypes::ModifyCursorLock);
  QTimer::singleShot(1, this, &ActionDialog::updateSize);
}

bool ActionDialog::isKeyAction(int index) const
{
  return ((index == ActionTypes::PressKey) || (index == ActionTypes::ReleaseKey) || (index == ActionTypes::ToggleKey));
}

bool ActionDialog::canSave() const
{
  if (isKeyAction(ui->comboActionType->currentIndex())) {
    const QList<QListWidgetItem *> items = ui->listComputers->findItems("*", Qt::MatchWildcard);
    int totalChecked = 0;
    for (const auto &item : items) {
      if (item->checkState() == Qt::Checked)
        totalChecked++;
    }
    return (!ui->keySequenceWidget->keySequence().toString().isEmpty() && (totalChecked > 0));
  }
  return true;
}

ActionDialog::~ActionDialog() = default;
