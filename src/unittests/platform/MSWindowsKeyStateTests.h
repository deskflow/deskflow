/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#pragma once

#include "arch/Arch.h"
#include "base/Log.h"

#include <QTest>

class MSWindowsKeyStateTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase();
  void remappedKeys_data();
  void remappedKeys();
  void virtualKeyOnlyInputHasStableButtons();

private:
  Arch m_arch;
  Log m_log;
};
