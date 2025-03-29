/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "HotkeyDialog.h"
#include "ui_HotkeyDialog.h"

HotkeyDialog::HotkeyDialog(QWidget *parent, Hotkey &hotkey)
    : QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
      ui{std::make_unique<Ui::HotkeyDialog>()},
      m_Hotkey(hotkey)
{
  ui->setupUi(this);

  ui->m_pKeySequenceWidgetHotkey->setText(m_Hotkey.text());
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
