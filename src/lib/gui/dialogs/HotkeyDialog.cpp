/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "HotkeyDialog.h"
#include "gui/DialogPresentation.h"
#include "ui_HotkeyDialog.h"

HotkeyDialog::HotkeyDialog(QWidget *parent, Hotkey &hotkey)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
      ui{std::make_unique<Ui::HotkeyDialog>()},
      m_Hotkey(hotkey)
{
  ui->setupUi(this);
  deskflow::gui::prepareFormDialog(this, ui->buttonBox, {560, 320});

  ui->m_pKeySequenceWidgetHotkey->setKeySequence(m_Hotkey.keySequence());
  ui->label->setWordWrap(true);
  const auto updateState = [this] {
    const bool valid = sequenceWidget()->valid();
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
    ui->label->setText(valid ? tr("Shortcut:") : tr("Record a shortcut to enable OK."));
  };
  connect(ui->m_pKeySequenceWidgetHotkey, &KeySequenceWidget::keySequenceChanged, this, updateState);
  updateState();
  if (!sequenceWidget()->valid())
    ui->m_pKeySequenceWidgetHotkey->setFocus();
}

HotkeyDialog::~HotkeyDialog() = default;

void HotkeyDialog::accept()
{
  if (!sequenceWidget()->valid())
    return;

  hotkey().setKeySequence(sequenceWidget()->keySequence());
  QDialog::accept();
}

const KeySequenceWidget *HotkeyDialog::sequenceWidget() const
{
  return ui->m_pKeySequenceWidgetHotkey;
}
