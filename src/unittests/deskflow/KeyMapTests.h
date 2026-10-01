/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "base/Log.h"

#include <QTest>

namespace deskflow {
class KeyMapTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void findBestKey_requiredDown_matchExactFirstItem();
  void findBestKey_requiredAndExtraSensitiveDown_matchExactFirstItem();
  void findBestKey_requiredAndExtraSensitiveDown_matchExactSecondItem();
  void findBestKey_extraSensitiveDown_matchExactSecondItem();
  void findBestKey_noRequiredDown_matchOneRequiredChangeItem();
  void findBestKey_onlyOneRequiredDown_matchTwoRequiredChangesItem();
  void findBestKey_noRequiredDown_cannotMatch();
  void isCommand();
  void mapkey();
  void parseModifiers_plusKey_keepsPlusAsKey();
  void parseKey_plusSymbol_parsesAsAsciiKey();
  void groupLanguages_preserveSourceIndices();
  void groupLanguages_keepCurrentDuplicate();
  void groupLanguages_imeOnlyKeepsCurrentGroup();
  void groupLanguages_emptyLanguageKeepsCurrentGroup();
  void groupLanguages_followKeymapSwap();
  void groupLanguages_emptyOverrideDoesNotUseInstalledList();
  void groupLanguages_legacyMappingIsPreserved();
  void mapKey_imeOnlyLanguageDoesNotSwitchGroup();

private:
  Log m_log;
};
} // namespace deskflow
