/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "ComputerLeaveTests.h"

#include "MockEventQueue.h"

#include "common/Settings.h"
#include "deskflow/Computer.h"
#include "deskflow/IPlatformComputer.h"

#include <QFile>

#include <utility>
#include <vector>

namespace {
//! Secondary platform computer that records the mouse buttons it is asked to fake.
class RecordingComputer : public IPlatformComputer
{
public:
  explicit RecordingComputer(const IEventQueue *events) : IPlatformComputer(events)
  {
  }

  std::vector<std::pair<ButtonID, bool>> m_buttons;

  void fakeMouseButton(ButtonID id, bool press) override
  {
    m_buttons.emplace_back(id, press);
  }

  // everything else is a no-op
  void *getEventTarget() const override
  {
    return nullptr;
  }
  bool getClipboard(ClipboardID, IClipboard *) const override
  {
    return false;
  }
  void getShape(int32_t &, int32_t &, int32_t &, int32_t &) const override
  {
  }
  void getCursorPos(int32_t &, int32_t &) const override
  {
  }
  void reconfigure(uint32_t) override
  {
  }
  uint32_t activeSides() override
  {
    return 0;
  }
  void warpCursor(int32_t, int32_t) override
  {
  }
  uint32_t registerHotKey(KeyID, KeyModifierMask) override
  {
    return 0;
  }
  void unregisterHotKey(uint32_t) override
  {
  }
  void fakeInputBegin() override
  {
  }
  void fakeInputEnd() override
  {
  }
  int32_t getJumpZoneSize() const override
  {
    return 0;
  }
  bool isAnyMouseButtonDown(uint32_t &) const override
  {
    return false;
  }
  void getCursorCenter(int32_t &, int32_t &) const override
  {
  }
  void fakeMouseMove(int32_t, int32_t) override
  {
  }
  void fakeMouseRelativeMove(int32_t, int32_t) const override
  {
  }
  void fakeMouseWheel(ScrollDelta) const override
  {
  }
  void updateKeyMap() override
  {
  }
  void updateKeyState() override
  {
  }
  void setHalfDuplexMask(KeyModifierMask) override
  {
  }
  void fakeKeyDown(KeyID, KeyModifierMask, KeyButton, const std::string &) override
  {
  }
  bool fakeKeyRepeat(KeyID, KeyModifierMask, int32_t, KeyButton, const std::string &) override
  {
    return false;
  }
  bool fakeKeyUp(KeyButton) override
  {
    return false;
  }
  void fakeAllKeysUp() override
  {
  }
  bool fakeCtrlAltDel() override
  {
    return false;
  }
  bool fakeMediaKey(KeyID) override
  {
    return false;
  }
  bool isKeyDown(KeyButton) const override
  {
    return false;
  }
  KeyModifierMask getActiveModifiers() const override
  {
    return 0;
  }
  KeyModifierMask pollActiveModifiers() const override
  {
    return 0;
  }
  int32_t pollActiveGroup() const override
  {
    return 0;
  }
  void pollPressedKeys(KeyButtonSet &) const override
  {
  }
  void enable() override
  {
  }
  void disable() override
  {
  }
  void enter() override
  {
  }
  bool canLeave() override
  {
    return true;
  }
  void leave() override
  {
  }
  bool setClipboard(ClipboardID, const IClipboard *) override
  {
    return false;
  }
  void checkClipboards() override
  {
  }
  void openScreensaver(bool) override
  {
  }
  void closeScreensaver() override
  {
  }
  void screensaver(bool) override
  {
  }
  void resetOptions() override
  {
  }
  void setOptions(const OptionsList &) override
  {
  }
  void setSequenceNumber(uint32_t) override
  {
  }
  std::string getSecureInputApp() const override
  {
    return {};
  }
  bool isPrimary() const override
  {
    return false;
  }
  void handleSystemEvent(const Event &) override
  {
  }
};
} // namespace

void ComputerLeaveTests::initTestCase()
{
  m_arch.init();
  QFile::remove(m_settingsFile);
  Settings::setSettingsFile(m_settingsFile);
}

void ComputerLeaveTests::disconnectReleasesHeldButton()
{
  MockEventQueue events;
  auto *platform = new RecordingComputer(&events); // owned by computer
  deskflow::Computer computer(platform, &events);
  computer.enable();
  computer.enter(0);

  // the connection drops mid-click, e.g. the client suspends (#10202)
  computer.mouseDown(kButtonLeft);
  computer.disable();

  QCOMPARE(platform->m_buttons.back(), std::make_pair(kButtonLeft, false));
}

void ComputerLeaveTests::disconnectSkipsReleasedButton()
{
  MockEventQueue events;
  auto *platform = new RecordingComputer(&events); // owned by computer
  deskflow::Computer computer(platform, &events);
  computer.enable();
  computer.enter(0);

  computer.mouseDown(kButtonLeft);
  computer.mouseUp(kButtonLeft);
  computer.disable();

  QCOMPARE(platform->m_buttons.size(), size_t{2});
}

QTEST_MAIN(ComputerLeaveTests)
