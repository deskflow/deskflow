/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
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
  void mapModifiersFromCarbon_carbonMask_returnsDeskflowMask_data();
  void mapModifiersFromCarbon_carbonMask_returnsDeskflowMask();
  void getKeyMap_asciiResource_preservesCarbonModifierRequirements_data();
  void getKeyMap_asciiResource_preservesCarbonModifierRequirements();
  void fakeKeyDown_zhuyinCapsSequence_preservesCapsAndShift();
  void fakeKeyDown_zhuyinCapsOff_clearsCapsModifier();
  void fakeKeyDown_zhuyinCapsOffLetter_keepsModifiersClear();
  void fakeKeyRepeat_shiftChanges_preservesCapsMode();
  void fakeKeyUp_repeatedZhuyinLetter_releasesKey();
  void fakeKeyRepeat_releasedZhuyinLetter_rejectsRepeat();
  void fakeKeyDown_inputModeChanges_usesCurrentCharacterPolicy();
  void fakeKeyDown_asciiInput_usesCharacterMapping_data();
  void fakeKeyDown_asciiInput_usesCharacterMapping();
  void fakeKeyDown_zhuyinLayoutSequence_preservesPhysicalKeyAndModifiers_data();
  void fakeKeyDown_zhuyinLayoutSequence_preservesPhysicalKeyAndModifiers();
  void fakeKeyDown_languageTargetsOtherGroup_usesOrdinaryMapping();
  void fakeKeyDown_zhuyinShortcut_usesOrdinaryMapping_data();
  void fakeKeyDown_zhuyinShortcut_usesOrdinaryMapping();
  void fakeKeyDown_nonCasePairResource_usesOrdinaryMapping();
  void fakeKeyDown_nonLetter_usesOrdinaryMapping();
  void fakePollShift();
  void fakePollChar();
  void fakePollCharWithModifier();

private:
  bool isKeyPressed(const OSXKeyState &keyState, KeyButton button);
  Arch m_arch;
  Log m_log;
};
