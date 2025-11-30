/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QTest>
#include "arch/Arch.h"
#include "base/Log.h"
#include "deskflow/AppUtil.h"

#if WINAPI_XWINDOWS
#include "platform/XWindowsScreen.h"
#include "base/EventQueue.h"
#endif

// Mock AppUtil for testing
class MockAppUtil final : public AppUtil
{
public:
  int run() override { return 0; }
  void startNode() override {}
  std::vector<std::string> getKeyboardLayoutList() override { return {}; }
  std::string getCurrentLanguageCode() override { return "en"; }
};

class XWindowsScreenTests final : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase();
  void cleanupTestCase();
  void testMonitorDetection();
  void testGetMonitors();
  void testTotalBounds();
  void testMonitorCount();
  void testPrimaryMonitor();

private:
  Arch m_arch;
  Log m_log;
  std::unique_ptr<MockAppUtil> m_appUtil;
#if WINAPI_XWINDOWS
  std::unique_ptr<EventQueue> m_events;
  std::unique_ptr<XWindowsScreen> m_screen;
#endif
};
