/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QObject>
#include <QTest>

#include "arch/Arch.h"
#include "base/Log.h"
#include "deskflow/KeyMap.h"

class KeyStateTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase();
  void keyDown();
  void keyUp();
  void invalidKey();
  void onKey_aKeyDown_keyStateOne();
  void onKey_aKeyUp_keyStateZero();
  void onKey_invalidKey_keyStateZero();
  void updateKeyState_pollDoesNothing_keyNotSet();
  void updateKeyState_activeModifiers_maskNotSet();
  void fakeKeyRepeat_invalidKey_returnsFalse();
  void fakeKeyUp_buttonNotDown_returnsFalse();
  void isKeyDown_noKeysDown_returnsFalse();
  void isKeyDown_keyDown_retrunsTrue();
  void updateKeyState_pollInsertsSingleKey_keyIsDown();

private:
  Arch m_arch;
  Log m_log;
  deskflow::KeyMap m_keymap;
};
