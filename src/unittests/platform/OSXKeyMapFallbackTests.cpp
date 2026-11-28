/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "arch/Arch.h"
#include "base/EventQueue.h"
#include "base/Log.h"
#include "platform/OSXKeyState.h"

#include <QTest>

class IMEOnlyKeyState : public OSXKeyState
{
public:
  using KeyState::getButton;
  using OSXKeyState::OSXKeyState;
  mutable bool foundIME = false;

protected:
  bool getGroups(AutoCFArray &groups) const override
  {
    std::lock_guard<std::mutex> lock(g_tisMutex);
    const void *keys[] = {kTISPropertyInputSourceType};
    const void *values[] = {kTISTypeKeyboardInputMode};
    AutoCFDictionary filter(
        CFDictionaryCreate(nullptr, keys, values, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks),
        CFRelease
    );
    AutoCFArray sources(TISCreateInputSourceList(filter.get(), true), CFRelease);
    if (sources) {
      for (CFIndex i = 0; i < CFArrayGetCount(sources.get()); ++i) {
        auto source = (TISInputSourceRef)CFArrayGetValueAtIndex(sources.get(), i);
        if (!TISGetInputSourceProperty(source, kTISPropertyUnicodeKeyLayoutData)) {
          const void *entry = source;
          groups = AutoCFArray(CFArrayCreate(nullptr, &entry, 1, &kCFTypeArrayCallBacks), CFRelease);
          foundIME = true;
          return true;
        }
      }
    }
    return false;
  }
};

class OSXKeyMapFallbackTests : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void initTestCase()
  {
    m_arch.init();
  }

  void imeWithoutLayoutStillMapsLetters()
  {
    EventQueue events;
    deskflow::KeyMap map;
    IMEOnlyKeyState state(&events, map, {"en"}, false);
    state.updateKeyMap();
    QVERIFY2(state.foundIME, "Test requires a system IME; it need not be enabled or selected");
    for (const char key : std::string("abcdefghijklmnopqrstuvwxyz0123456789"))
      QVERIFY2(state.getButton(key, 0) != 0, "An IME with no layout must still map ordinary characters");
    QVERIFY(state.getButton(kKeyShift_L, 0) != 0);
  }

  void rebuildingKeyMapKeepsFallback()
  {
    EventQueue events;
    deskflow::KeyMap map;
    IMEOnlyKeyState state(&events, map, {"en"}, false);
    state.updateKeyMap();
    QVERIFY(state.foundIME);
    const auto button = state.getButton('a', 0);
    QVERIFY(button != 0);
    state.updateKeyMap();
    QCOMPARE(state.getButton('a', 0), button);
  }

private:
  Arch m_arch;
  Log m_log;
};

QTEST_MAIN(OSXKeyMapFallbackTests)
#include "OSXKeyMapFallbackTests.moc"
