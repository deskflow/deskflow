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
  m_log.setFilter(LogLevel::Debug2);
}

void KeyboardLayoutManagerTests::remoteLayouts()
{
  std::string remoteLayouts = "00000419,00000409,00000422";
  deskflow::KeyboardLayoutManager manager({"00000419", "00000409", "00000422"});

  manager.setRemoteLayouts(remoteLayouts);
  QCOMPARE(manager.getRemoteLayouts(), (std::vector<std::string>{"00000419", "00000409", "00000422"}));

  manager.setRemoteLayouts("00020409");
  QCOMPARE(manager.getRemoteLayouts(), (std::vector<std::string>{"00020409"}));

  manager.setRemoteLayouts("ruenuk");
  QCOMPARE(manager.getRemoteLayouts(), (std::vector<std::string>{"ru", "en", "uk"}));

  manager.setRemoteLayouts(std::string());
  QVERIFY(manager.getRemoteLayouts().empty());
}

void KeyboardLayoutManagerTests::localLayout()
{
  std::vector<std::string> localLayouts = {"00000419", "00000409", "00000422"};
  deskflow::KeyboardLayoutManager manager(localLayouts);
  QCOMPARE(manager.getLocalLayouts(), (std::vector<std::string>{"00000419", "00000409", "00000422"}));
}

void KeyboardLayoutManagerTests::missedLayout()
{
  std::string remoteLayouts = "00000419,00000409,00000422";
  std::vector<std::string> localLayouts = {"00000409"};
  deskflow::KeyboardLayoutManager manager(localLayouts);

  manager.setRemoteLayouts(remoteLayouts);
  QCOMPARE(manager.getMissedLayouts(), "00000419, 00000422");
}

void KeyboardLayoutManagerTests::layoutInstall()
{
  std::vector<std::string> localLayouts = {"00000419", "00000409", "00000422"};
  deskflow::KeyboardLayoutManager manager(localLayouts);

  QVERIFY(!manager.isLayoutInstalled("00020409"));
  QVERIFY(manager.isLayoutInstalled("00000409"));
}

void KeyboardLayoutManagerTests::serializeLocalLayouts()
{
  std::vector<std::string> localLayouts = {"00000419", "00000409", "00000422"};
  deskflow::KeyboardLayoutManager manager(localLayouts);

  QCOMPARE(manager.getSerializedLocalLayouts(), "00000419,00000409,00000422");
}

QTEST_MAIN(KeyboardLayoutManagerTests)
