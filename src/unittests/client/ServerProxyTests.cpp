/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ServerProxyTests.h"

#include "client/ServerProxy.h"

#include <array>

namespace {

std::array<KeyModifierID, kKeyModifierIDLast> identityModifierMap()
{
  std::array<KeyModifierID, kKeyModifierIDLast> modifiers;
  for (KeyModifierID id = 0; id < kKeyModifierIDLast; ++id) {
    modifiers[id] = id;
  }
  return modifiers;
}

} // namespace

void ServerProxyTests::translateKey_regularKey_returnsOriginalKey()
{
  auto modifiers = identityModifierMap();

  QCOMPARE(ServerProxy::translateKey('p', modifiers.data()), static_cast<KeyID>('p'));
}

void ServerProxyTests::translateKey_mappedModifier_clampsOnlyModifierIndex()
{
  auto modifiers = identityModifierMap();
  modifiers[kKeyModifierIDShift] = kKeyModifierIDSuper;

  QCOMPARE(ServerProxy::translateKey(kKeyShift_L, modifiers.data()), kKeySuper_L);
}

void ServerProxyTests::translateModifierMask_lockModifier_preservesHighBits()
{
  auto modifiers = identityModifierMap();

  QCOMPARE(ServerProxy::translateModifierMask(KeyModifierNumLock, modifiers.data()), KeyModifierNumLock);
}

void ServerProxyTests::translateModifierMask_mappedModifier_preservesUnmappedBits()
{
  auto modifiers = identityModifierMap();
  modifiers[kKeyModifierIDControl] = kKeyModifierIDAlt;

  const auto mask = KeyModifierControl | KeyModifierNumLock;
  const auto expectedMask = KeyModifierAlt | KeyModifierNumLock;
  QCOMPARE(ServerProxy::translateModifierMask(mask, modifiers.data()), expectedMask);
}

QTEST_MAIN(ServerProxyTests)
