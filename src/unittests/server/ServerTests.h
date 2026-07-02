/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QTest>

#include "arch/Arch.h"
#include "base/Log.h"

class ServerTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase();
  void SwitchToScreenInfo_alloc_screen();
  void KeyboardBroadcastInfo_alloc_stateAndSceens();
  void onKeyDown_altGrRemap_usesBaseKey();
  void onKeyDown_withoutAltGrRemap_preservesTranslatedKey();
  void onKeyRepeat_altGrRemap_usesBaseKey();

private:
  Arch m_arch;
  Log m_log;
};
