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
