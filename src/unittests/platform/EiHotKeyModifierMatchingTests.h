/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QTest>

namespace deskflow {

class EiHotKeyModifierMatchingTests : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void findByMask_lockModifiers_matchHotkey_data();
  void findByMask_lockModifiers_matchHotkey();
  void findByMask_differentCommandModifiers_doNotMatch_data();
  void findByMask_differentCommandModifiers_doNotMatch();
  void findByMask_multipleHotkeys_selectsMatchingModifiers();
};

} // namespace deskflow
