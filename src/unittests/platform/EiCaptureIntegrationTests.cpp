/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "EiCaptureIntegrationTests.h"

#include "EiTestFixtures.h"
#include "base/EventQueue.h"
#include "platform/EiKeyState.h"

#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <libeis.h>

using ei_test::CaptureKeymap;
using ei_test::TestAppUtil;

void EiCaptureIntegrationTests::initTestCase()
{
  m_arch.init();
  m_log.setFilter(LogLevel::Level::Verbose);
}

void EiCaptureIntegrationTests::eisCaptureContract()
{
  TestAppUtil util;
  class RecordingQueue : public EventQueue
  {
  public:
    struct KeyEvent
    {
      EventTypes type;
      KeyID key;
      KeyModifierMask mask;
    };
    std::vector<KeyEvent> keys;
    void addEvent(Event &&event) override
    {
      if (event.getType() == EventTypes::KeyStateKeyDown || event.getType() == EventTypes::KeyStateKeyUp ||
          event.getType() == EventTypes::KeyStateKeyRepeat) {
        const auto *info = static_cast<const IKeyState::KeyInfo *>(event.getData());
        keys.push_back({event.getType(), info->m_key, info->m_mask});
      }
      EventQueue::addEvent(std::move(event));
    }
  } events;

  QTemporaryDir directory;
  QVERIFY(directory.isValid());
  const auto socket = (directory.path() + "/eis.sock").toLocal8Bit();
  auto *server = eis_new(nullptr);
  QVERIFY(server);
  eis_seat *seat = nullptr;
  eis_client *client = nullptr;
  std::vector<eis_device *> keyboards;
  const auto cleanup = qScopeGuard([&] {
    for (auto *keyboard : keyboards)
      eis_device_unref(keyboard);
    eis_seat_unref(seat);
    eis_client_unref(client);
    eis_unref(server);
  });
  QCOMPARE(eis_setup_backend_socket(server, socket.constData()), 0);
  const auto previousSocket = qgetenv("LIBEI_SOCKET");
  const auto restoreEnvironment = qScopeGuard([&] {
    if (previousSocket.isNull())
      qunsetenv("LIBEI_SOCKET");
    else
      qputenv("LIBEI_SOCKET", previousSocket);
  });
  qputenv("LIBEI_SOCKET", socket);
  deskflow::EiComputer computer(true, &events, false);

  QTemporaryFile keymap;
  QVERIFY(keymap.open());
  QCOMPARE(keymap.write(CaptureKeymap), qint64(sizeof(CaptureKeymap) - 1));
  QVERIFY(keymap.flush());
  const auto addKeyboard = [&](const char *name) {
    auto *keyboard = eis_seat_new_device(seat);
    eis_device_configure_name(keyboard, name);
    eis_device_configure_type(keyboard, EIS_DEVICE_TYPE_VIRTUAL);
    eis_device_configure_capability(keyboard, EIS_DEVICE_CAP_KEYBOARD);
    auto *map = eis_device_new_keymap(keyboard, EIS_KEYMAP_TYPE_XKB, keymap.handle(), keymap.size());
    eis_keymap_add(map);
    eis_keymap_unref(map);
    eis_device_add(keyboard);
    eis_device_resume(keyboard);
    keyboards.push_back(keyboard);
  };
  const auto pump = [&] {
    eis_dispatch(server);
    while (auto *event = eis_get_event(server)) {
      switch (eis_event_get_type(event)) {
      case EIS_EVENT_CLIENT_CONNECT:
        client = eis_client_ref(eis_event_get_client(event));
        eis_client_connect(client);
        seat = eis_client_new_seat(client, "capture-test");
        eis_seat_configure_capability(seat, EIS_DEVICE_CAP_KEYBOARD);
        eis_seat_add(seat);
        break;
      case EIS_EVENT_SEAT_BIND:
        if (keyboards.empty() && eis_event_seat_has_capability(event, EIS_DEVICE_CAP_KEYBOARD)) {
          addKeyboard("tracked");
          addKeyboard("ignored");
        }
        break;
      default:
        break;
      }
      eis_event_unref(event);
    }
    computer.handleSystemEvent(Event{});
  };
  QTRY_VERIFY_WITH_TIMEOUT((pump(), computer.m_eiDevices.size() == 2), 5000);
  auto *tracked = computer.m_eiKeyboard;
  QVERIFY(tracked);
  const auto sendKey = [&](eis_device *keyboard, uint32_t code, bool pressed) {
    eis_device_keyboard_key(keyboard, code - 8, pressed);
    eis_device_frame(keyboard, eis_now(server));
  };
  auto *keyboard = keyboards[0];
  auto *extra = keyboards[1];
  auto &state = *computer.m_keyState;

  // VT-switch presses, repeats and releases must not produce network key events.
  // A snapshot can arrive before START_EMULATING. The first key must use it.
  eis_device_keyboard_send_xkb_modifiers(keyboard, 4u | 8u, 0, 0, 1);
  eis_device_start_emulating(keyboard, 1);
  for (uint32_t code = 67; code < 79; ++code) {
    sendKey(keyboard, code, true);
    sendKey(keyboard, code, true);
    sendKey(keyboard, code, false);
  }
  eis_device_keyboard_send_xkb_modifiers(keyboard, 0, 0, 0, 1);
  sendKey(keyboard, 24, true);
  QTRY_VERIFY_WITH_TIMEOUT((pump(), events.keys.size() == 1), 5000);
  QCOMPARE(events.keys.back().key, KeyID(0x0439));
  QCOMPARE(events.keys.back().type, EventTypes::KeyStateKeyDown);

  // Queue key -> frame -> modifiers -> key together. Do not defer the snapshot
  // to another frame or let inferred key actions change authoritative state.
  sendKey(keyboard, 50, true);
  eis_device_keyboard_send_xkb_modifiers(keyboard, 1, 0, 0, 1);
  sendKey(keyboard, 24, false);
  QTRY_VERIFY_WITH_TIMEOUT((pump(), events.keys.size() == 3), 5000);
  QCOMPARE(events.keys[1].mask & KeyModifierShift, KeyModifierMask{0});
  QCOMPARE(events.keys[2].key, KeyID(0x0419));
  QVERIFY(events.keys[2].mask & KeyModifierShift);

  eis_device_start_emulating(extra, 1);
  eis_device_keyboard_send_xkb_modifiers(extra, 0, 0, 0, 0);
  sendKey(extra, 24, true);
  eis_device_stop_emulating(extra);
  eis_device_pause(extra);
  eis_device_remove(extra);
  QTRY_VERIFY_WITH_TIMEOUT((pump(), computer.m_eiDevices.size() == 1), 5000);
  QCOMPARE(computer.m_eiKeyboard, tracked);
  QCOMPARE(events.keys.size(), size_t{3});
  QCOMPARE(state.mapKeyFromKeyval(24), KeyID(0x0419));

  sendKey(keyboard, 24, true);
  QTRY_VERIFY_WITH_TIMEOUT((pump(), events.keys.size() == 4), 5000);
  eis_device_stop_emulating(keyboard);
  QTRY_VERIFY_WITH_TIMEOUT((pump(), !state.m_haveCaptureModifiers), 5000);
  QCOMPARE(state.pollActiveModifiers(), KeyModifierMask{0});
  QCOMPARE(computer.m_lastPressed, KeyID(kKeyNone));
  QCOMPARE(state.mapKeyFromKeyval(24), KeyID('q'));

  // The alternative ordering: START_EMULATING followed by a fresh snapshot.
  eis_device_start_emulating(keyboard, 2);
  eis_device_keyboard_send_xkb_modifiers(keyboard, 0, 0, 0, 1);
  sendKey(keyboard, 24, true);
  QTRY_VERIFY_WITH_TIMEOUT((pump(), events.keys.size() == 5), 5000);
  QCOMPARE(events.keys.back().key, KeyID(0x0439));
  QCOMPARE(events.keys.back().type, EventTypes::KeyStateKeyDown);
  // libeis 1.3 requires stopping emulation before pausing a receiver device.
  // Install another snapshot while resumed to verify pause invalidates it too.
  eis_device_stop_emulating(keyboard);
  eis_device_keyboard_send_xkb_modifiers(keyboard, 1, 0, 16, 1);
  QTRY_VERIFY_WITH_TIMEOUT((pump(), state.pollActiveModifiers() & KeyModifierNumLock), 5000);
  eis_device_pause(keyboard);
  QTRY_VERIFY_WITH_TIMEOUT((pump(), !state.m_haveCaptureModifiers), 5000);
  QCOMPARE(state.pollActiveModifiers(), KeyModifierMask{0});
  QCOMPARE(computer.m_lastPressed, KeyID(kKeyNone));
  eis_device_resume(keyboard);
  eis_device_keyboard_send_xkb_modifiers(keyboard, 0, 0, 0, 1);
  eis_device_start_emulating(keyboard, 3);
  sendKey(keyboard, 24, true);
  QTRY_VERIFY_WITH_TIMEOUT((pump(), events.keys.size() == 6), 5000);
  QCOMPARE(events.keys.back().key, KeyID(0x0439));
  QCOMPARE(events.keys.back().type, EventTypes::KeyStateKeyDown);

  eis_device_remove(keyboard);
  QTRY_VERIFY_WITH_TIMEOUT((pump(), computer.m_eiKeyboard == nullptr), 5000);
  QVERIFY(!state.m_haveCaptureModifiers);
  QCOMPARE(computer.m_lastPressed, KeyID(kKeyNone));
  addKeyboard("replacement");
  QTRY_VERIFY_WITH_TIMEOUT((pump(), computer.m_eiKeyboard != nullptr), 5000);
  QCOMPARE(state.mapKeyFromKeyval(24), KeyID('q'));
  eis_device_keyboard_send_xkb_modifiers(keyboards.back(), 1, 0, 16, 1);
  QTRY_VERIFY_WITH_TIMEOUT((pump(), state.m_haveCaptureModifiers), 5000);
  eis_client_disconnect(client);
  QTRY_VERIFY_WITH_TIMEOUT((pump(), computer.m_eiKeyboard == nullptr), 5000);
  QVERIFY(!state.m_haveCaptureModifiers);
  QCOMPARE(state.pollActiveModifiers(), KeyModifierMask{0});
  QCOMPARE(computer.m_lastPressed, KeyID(kKeyNone));
}

QTEST_MAIN(EiCaptureIntegrationTests)
