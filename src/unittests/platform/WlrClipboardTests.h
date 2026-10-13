/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/Arch.h"
#include "base/Log.h"
#include "platform/WlrClipboard.h"

#include <QTest>
#include <memory>

class WlrClipboardTests : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void initTestCase();
  void testAvailability();
  void testOpenAndEmpty();
  void testAddAndGetText();
  void cleanupTestCase();

private:
  Arch m_arch;
  Log m_log;
  std::unique_ptr<deskflow::WlrClipboard> m_clipboard;
};
