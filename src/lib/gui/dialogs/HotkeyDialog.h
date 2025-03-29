/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2008 Volker Lanz <vl@fidra.de>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "Hotkey.h"

#include <QDialog>

class KeySequenceWidget;

namespace Ui {
class HotkeyDialog;
}

class HotkeyDialog : public QDialog
{
  Q_OBJECT

public:
  HotkeyDialog(QWidget *parent, Hotkey &hotkey);
  ~HotkeyDialog() override;

public:
  const Hotkey &hotkey() const
  {
    return m_Hotkey;
  }

protected slots:
  void accept() override;

protected:
  const KeySequenceWidget *sequenceWidget() const;

  Hotkey &hotkey()
  {
    return m_Hotkey;
  }

private:
  std::unique_ptr<Ui::HotkeyDialog> ui;
  Hotkey &m_Hotkey;
};
