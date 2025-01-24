/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ActionDialog.h"
#include "ui_ActionDialog.h"

#include "Action.h"
#include "Hotkey.h"
#include "KeySequence.h"
#include "ServerConfig.h"

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
  connect(ui->listScreens, &QListWidget::itemChanged, this, [&] {
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(canSave());
  });
  connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ActionDialog::accept);
  connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ActionDialog::reject);

  ui->keySequenceWidget->setText(m_action.keySequence().toString());
  ui->keySequenceWidget->setKeySequence(m_action.keySequence());

  ui->comboSwitchInDirection->setCurrentIndex(m_action.switchDirection());
  ui->comboLockCursorToScreen->setCurrentIndex(m_action.lockCursorMode());

  ui->comboActionType->setCurrentIndex(m_action.type());
  ui->comboTriggerOn->setCurrentIndex(m_action.activeOnRelease());

  for (const Screen &screen : config.screens()) {
    if (screen.isNull())
      continue;
    auto *newListItem = new QListWidgetItem(screen.name());
    newListItem->setCheckState(Qt::Checked);
    if ((m_action.typeScreenNames().indexOf(screen.name()) == -1) &&
        (m_action.haveScreens() && !m_action.typeScreenNames().isEmpty()))
      newListItem->setCheckState(Qt::Unchecked);

    ui->listScreens->addItem(newListItem);

    ui->comboSwitchToScreen->addItem(tr("Switch to %1").arg(screen.name()));
    if (screen.name() == m_action.switchScreenName())
      ui->comboSwitchToScreen->setCurrentIndex(ui->comboSwitchToScreen->count() - 1);
  }

  ui->keySequenceWidget->setVisible(false);
  ui->groupScreens->setVisible(false);
  ui->listScreens->setEnabled(!ui->keySequenceWidget->keySequence().isMouseButton());
  ui->comboSwitchToScreen->setVisible(false);
  ui->comboSwitchInDirection->setVisible(false);
  ui->comboLockCursorToScreen->setVisible(false);

  actionTypeChanged(ui->comboActionType->currentIndex());
}

void ActionDialog::accept()
{
  if (!canSave())
    return;

  m_action.setKeySequence(ui->keySequenceWidget->keySequence());
  m_action.setType(ui->comboActionType->currentIndex());

  m_action.typeScreenNames().clear();

  int screenCount = ui->listScreens->count();

  for (int i = 0; i < ui->listScreens->count(); i++) {
    const auto &item = ui->listScreens->item(i);
    m_action.typeScreenNames().append(item->text());
    if (item->checkState() == Qt::Unchecked) {
      screenCount--;
      m_action.typeScreenNames().removeLast();
    }
  }

  if (screenCount == ui->listScreens->count())
    m_action.typeScreenNames().clear();

  m_action.setHaveScreens(screenCount);

  m_action.setSwitchScreenName(ui->comboSwitchToScreen->currentText().remove(tr("Switch to ")));
  m_action.setSwitchDirection(ui->comboSwitchInDirection->currentIndex());
  m_action.setLockCursorMode(ui->comboLockCursorToScreen->currentIndex());
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
  ui->listScreens->setEnabled(!ui->keySequenceWidget->keySequence().isMouseButton());
  ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(canSave());
}

void ActionDialog::actionTypeChanged(int index)
{
  ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(canSave());
  ui->keySequenceWidget->setVisible(isKeyAction(index));
  ui->groupScreens->setVisible(isKeyAction(index));
  ui->listScreens->setEnabled(!ui->keySequenceWidget->keySequence().isMouseButton());
  ui->comboSwitchToScreen->setVisible(index == ActionTypes::SwitchTo);
  ui->comboSwitchInDirection->setVisible(index == ActionTypes::SwitchInDirection);
  ui->comboLockCursorToScreen->setVisible(index == ActionTypes::ModifyCursorLock);
  QTimer::singleShot(1, this, &ActionDialog::updateSize);
}

bool ActionDialog::isKeyAction(int index)
{
  return ((index == ActionTypes::PressKey) || (index == ActionTypes::ReleaseKey) || (index == ActionTypes::ToggleKey));
}

bool ActionDialog::canSave()
{
  if (isKeyAction(ui->comboActionType->currentIndex())) {
    const QList<QListWidgetItem *> items = ui->listScreens->findItems("*", Qt::MatchWildcard);
    int totalChecked = 0;
    for (const auto &item : items) {
      if (item->checkState() == Qt::Checked)
        totalChecked++;
    };
    return (!ui->keySequenceWidget->keySequence().toString().isEmpty() && (totalChecked > 0));
  }
  return true;
}

ActionDialog::~ActionDialog() = default;
