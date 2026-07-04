/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QTest>

class ServerProxyTests : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void translateKey_regularKey_returnsOriginalKey();
  void translateKey_mappedModifier_clampsOnlyModifierIndex();
  void translateModifierMask_lockModifier_preservesHighBits();
  void translateModifierMask_mappedModifier_preservesUnmappedBits();
};
