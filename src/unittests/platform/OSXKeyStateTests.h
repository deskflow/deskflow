/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "base/Log.h"

#include "arch/Arch.h"
#include "platform/OSXKeyState.h"

#include <QTest>

class OSXKeyStateTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase();
  // Test are run in order top to bottom
  void mapModifiersFromOSX_OSXMask();
  void fakePollShift();
  void fakePollChar();
  void fakePollCharWithModifier();
  void getKeyMap_addsLayoutSpecificDeadKeyOutput();
  void getKeyMap_usesDeadKeyThatProducesSpacingOutput();
  void getKeyMap_distinguishesDeadKeyOutputsByModifier();
  void getKeyMap_doesNotAssumeDeadKeyOutput();
  void getKeyMap_preservesDirectDeadKeyOutput();
  void sendKeyEvents_multiCharacterButton_releasesIntermediateKey();

private:
  bool isKeyPressed(const OSXKeyState &keyState, KeyButton button);
  Arch m_arch;
  Log m_log;
};
