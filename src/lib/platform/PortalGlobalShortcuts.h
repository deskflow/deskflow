/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/IEventQueue.h"
#include "mt/Thread.h"

#include <glib.h>
#include <libportal/globalshortcuts.h>
#include <optional>
#include <xkbcommon/xkbcommon.h>

#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "deskflow/KeyTypes.h"

namespace deskflow {

class EiScreen;

class PortalGlobalShortcuts
{
public:
  PortalGlobalShortcuts(EiScreen *screen, IEventQueue *events);
  ~PortalGlobalShortcuts();
  struct HotKey
  {
    std::uint32_t id = 0;
    KeyID key = kKeyNone;
    KeyModifierMask mask = 0;
  };

  void setHotKeys(std::vector<HotKey> hotkeys);

private:
  void glibThread(const void *);
  void setupSession(XdpGlobalShortcutsSession *session);

  void createSessionDone(GObject *object, GAsyncResult *res);
  void bindShortcutsDone(GObject *object, GAsyncResult *res);

  void handleSessionClosed(XdpSession *session);
  void handleActivated(
      XdpGlobalShortcutsSession *session, const char *shortcutId, const guint64 timestamp, const GVariant *options
  );
  void handleDeactivated(
      XdpGlobalShortcutsSession *session, const char *shortcutId, const guint64 timestamp, const GVariant *options
  );
  void handleShortcutsChanged(XdpGlobalShortcutsSession *session, GPtrArray *shortcuts);

  /// g_signal_connect callback wrapper
  static void sessionClosed(XdpSession *session, const gpointer data)
  {
    static_cast<PortalGlobalShortcuts *>(data)->handleSessionClosed(session);
  }

  static void activated(
      XdpGlobalShortcutsSession *session, const char *shortcutId, const guint64 timestamp, const GVariant *options,
      const gpointer data
  )
  {
    static_cast<PortalGlobalShortcuts *>(data)->handleActivated(session, shortcutId, timestamp, options);
  }

  static void deactivated(
      XdpGlobalShortcutsSession *session, const char *shortcutId, const guint64 timestamp, const GVariant *options,
      const gpointer data
  )
  {
    static_cast<PortalGlobalShortcuts *>(data)->handleDeactivated(session, shortcutId, timestamp, options);
  }

  static void shortcutsChanged(XdpGlobalShortcutsSession *session, GPtrArray *shortcuts, const gpointer data)
  {
    static_cast<PortalGlobalShortcuts *>(data)->handleShortcutsChanged(session, shortcuts);
  }

  enum class Signal : uint8_t
  {
    SessionClosed,
    Activated,
    Deactivated,
    ShortcutsChanged,
  };

  struct Binding
  {
    std::uint32_t id = 0;
    KeyID key = kKeyNone;
    KeyModifierMask mask = 0;
    std::string shortcutId;
    std::string description;
    std::string preferredTrigger;
  };

  static std::optional<xkb_keysym_t> deskflowKeyToXkbKeysym(KeyID key);
  static std::string keyNameForPreferredTrigger(KeyID key);
  static std::string makePreferredTrigger(KeyID key, KeyModifierMask mask);
  static bool isValidPreferredTriggerIdentifier(const std::string &name);

  gboolean createSession();
  void closeSession();
  void bindShortcuts();

  std::uint32_t hotKeyIdForShortcutId(const char *shortcutId) const;
  static Binding makeBinding(const HotKey &hotkey);
  std::vector<Binding> bindingsSnapshot() const;
  static std::string makeShortcutId(KeyID key, KeyModifierMask mask);

  EiScreen *m_screen = nullptr;
  IEventQueue *m_events = nullptr;

  Thread *m_glibThread = nullptr;
  GMainLoop *m_glibMainLoop = nullptr;

  XdpPortal *m_portal = nullptr;
  XdpGlobalShortcutsSession *m_session = nullptr;
  GCancellable *m_cancellable = nullptr;

  std::map<Signal, gulong> m_signals = {
      {Signal::SessionClosed, 0},
      {Signal::Activated, 0},
      {Signal::Deactivated, 0},
      {Signal::ShortcutsChanged, 0},
  };

  mutable std::mutex m_bindingsMutex;
  guint m_pendingIdleSource = 0; // guarded by m_bindingsMutex
  // Snapshot derived from EiScreen::m_hotkeys. This is portal-facing state
  std::vector<Binding> m_bindings;
};

} // namespace deskflow
