/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2014 - 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "KeyboardLayoutManagerTests.h"

#include "deskflow/KeyboardLayoutManager.h"

void KeyboardLayoutManagerTests::initTestCase()
{
  m_log.setFilter(LogLevel::Verbose);
}

void KeyboardLayoutManagerTests::remoteLayouts()
{
  std::string remoteLayouts = "ruenuk";
  deskflow::KeyboardLayoutManager manager({"ru", "en", "uk"});

  manager.setRemoteLayouts(remoteLayouts);
  QCOMPARE(manager.getRemoteLayouts(), (std::vector<std::string>{"ru", "en", "uk"}));

  manager.setRemoteLayouts(std::string());
  QVERIFY(manager.getRemoteLayouts().empty());
}

void KeyboardLayoutManagerTests::localLayout()
{
  std::vector<std::string> localLayouts = {"ru", "en", "uk"};
  deskflow::KeyboardLayoutManager manager(localLayouts);
  QCOMPARE(manager.getLocalLayouts(), (std::vector<std::string>{"ru", "en", "uk"}));
}

void KeyboardLayoutManagerTests::missedLayout()
{
  std::string remoteLayouts = "ruenuk";
  std::vector<std::string> localLayouts = {"en"};
  deskflow::KeyboardLayoutManager manager(localLayouts);

  manager.setRemoteLayouts(remoteLayouts);
  QCOMPARE(manager.getMissedLayouts(), "ru, uk");
}

void KeyboardLayoutManagerTests::layoutInstall()
{
  std::vector<std::string> localLayouts = {"ru", "en", "uk"};
  deskflow::KeyboardLayoutManager manager(localLayouts);

  QVERIFY(!manager.isLayoutInstalled("us"));
  QVERIFY(manager.isLayoutInstalled("en"));
}

void KeyboardLayoutManagerTests::serializeLocalLayouts()
{
  std::vector<std::string> localLayouts = {"ru", "en", "uk"};
  deskflow::KeyboardLayoutManager manager(localLayouts);

  QCOMPARE(manager.getSerializedLocalLayouts(), "ruenuk");
}

QTEST_MAIN(KeyboardLayoutManagerTests)
