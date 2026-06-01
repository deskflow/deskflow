/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QObject>

class MSWindowsHookIntegrationTests : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void deadKeyIsPreservedAcrossUnrelatedKeyUpUntilNextComposeKeyDown_data();
  void deadKeyIsPreservedAcrossUnrelatedKeyUpUntilNextComposeKeyDown();
  void deadKeyCompose_whenDeadReleasedBeforeComposeKeyDown_data();
  void deadKeyCompose_whenDeadReleasedBeforeComposeKeyDown();
  void deadKeyCompose_whenDeadReleasedBetweenComposeKeyDownAndUp_data();
  void deadKeyCompose_whenDeadReleasedBetweenComposeKeyDownAndUp();
  void deadKeyDoesNotLeakToNextCharacter_afterCompose_data();
  void deadKeyDoesNotLeakToNextCharacter_afterCompose();
  void deadKeyDoesNotLeak_whenNextKeyDownBeforeComposeKeyUp_data();
  void deadKeyDoesNotLeak_whenNextKeyDownBeforeComposeKeyUp();
  void altGrMappedCharacter_isPreservedInRelayPath_data();
  void altGrMappedCharacter_isPreservedInRelayPath();
  void ctrlAltFallback_setsNoAltGrFlagForPlainCharacter_data();
  void ctrlAltFallback_setsNoAltGrFlagForPlainCharacter();
};
