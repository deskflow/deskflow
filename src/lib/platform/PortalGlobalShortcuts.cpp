/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/PortalGlobalShortcuts.h"
#include "base/Log.h"
#include "base/TMethodJob.h"
#include "platform/EiScreen.h"
#include <inttypes.h>

namespace deskflow {

PortalGlobalShortcuts::PortalGlobalShortcuts(EiScreen *screen, IEventQueue *events)
    : m_screen{screen},
      m_events{events},
      m_portal{xdp_portal_new()}
{
  m_glibMainLoop = g_main_loop_new(nullptr, true);

  auto tMethodJob = new TMethodJob<PortalGlobalShortcuts>(this, &PortalGlobalShortcuts::glibThread);
  m_glibThread = new Thread(tMethodJob);
}

PortalGlobalShortcuts::~PortalGlobalShortcuts()
{
  if (m_glibMainLoop && g_main_loop_is_running(m_glibMainLoop)) {
    g_main_loop_quit(m_glibMainLoop);
  }

  if (m_glibThread) {
    m_glibThread->cancel();
    m_glibThread->wait();
    delete m_glibThread;
    m_glibThread = nullptr;
  }

  if (m_glibMainLoop) {
    g_main_loop_unref(m_glibMainLoop);
    m_glibMainLoop = nullptr;
  }

  closeSession();

  if (m_portal) {
    g_object_unref(m_portal);
    m_portal = nullptr;
  }
}

void PortalGlobalShortcuts::setHotKeys(std::vector<HotKey> hotkeys)
{
  std::vector<Binding> bindings;
  bindings.reserve(hotkeys.size());

  for (const auto &hotkey : hotkeys) {
    bindings.push_back(makeBinding(hotkey));
  }

  {
    std::scoped_lock lock{m_bindingsMutex};
    m_bindings = std::move(bindings);
  }

  scheduleRecreateSession();
}

void PortalGlobalShortcuts::clearHotKeys()
{
  {
    std::scoped_lock lock{m_bindingsMutex};
    m_bindings.clear();
  }

  scheduleRecreateSession();
}

PortalGlobalShortcuts::Binding PortalGlobalShortcuts::makeBinding(const HotKey &hotkey)
{
  Binding binding;

  binding.id = hotkey.id;
  binding.key = hotkey.key;
  binding.mask = hotkey.mask;
  binding.shortcutId = makeShortcutId(hotkey.key, hotkey.mask);
  // This is not the prettiest way, but we currently have no direct access to the Hotkey object
  // of lib/gui/Hotkey.cpp, so we cannot use Hotkey::text()
  const std::string trigger = makePreferredTrigger(hotkey.key, hotkey.mask);
  binding.description = "Hotkey " + trigger;
  binding.preferredTrigger = trigger;

  return binding;
}

std::vector<PortalGlobalShortcuts::Binding> PortalGlobalShortcuts::bindingsSnapshot() const
{
  std::scoped_lock lock{m_bindingsMutex};
  return m_bindings;
}

std::uint32_t PortalGlobalShortcuts::hotKeyIdForShortcutId(const char *shortcutId) const
{
  std::scoped_lock lock{m_bindingsMutex};

  const auto it = std::find_if(m_bindings.begin(), m_bindings.end(), [shortcutId](const Binding &binding) {
    return binding.shortcutId == shortcutId;
  });

  if (it == m_bindings.end()) {
    return 0;
  }

  return it->id;
}

gboolean PortalGlobalShortcuts::recreateSessionIdle(gpointer data)
{
  static_cast<PortalGlobalShortcuts *>(data)->recreateSession();
  return G_SOURCE_REMOVE;
}

void PortalGlobalShortcuts::recreateSession()
{
  closeSession();

  if (bindingsSnapshot().empty()) {
    LOG_DEBUG("no global shortcuts to bind");
    return;
  }

  LOG_DEBUG("creating global shortcuts session");

  xdp_portal_create_global_shortcuts_session(
      m_portal,
      nullptr, // cancellable
      [](GObject *obj, GAsyncResult *res, gpointer data) {
        static_cast<PortalGlobalShortcuts *>(data)->handleInitSession(obj, res);
      },
      this
  );
}

void PortalGlobalShortcuts::scheduleRecreateSession()
{
  if (m_recreateSessionSource != 0) {
    return;
  }

  // Schedule a session recreation after a short delay to avoid multiple calls
  m_recreateSessionSource = g_timeout_add(100, recreateSessionIdle, this);
}

void PortalGlobalShortcuts::closeSession()
{
  if (!m_session) {
    return;
  }

  using enum Signal;

  if (XdpSession *parentSession = xdp_global_shortcuts_session_get_session(m_session)) {
    if (m_signals.at(SessionClosed)) {
      g_signal_handler_disconnect(parentSession, m_signals.at(SessionClosed));
      m_signals.at(SessionClosed) = 0;
    }
  }

  if (m_signals.at(Activated)) {
    g_signal_handler_disconnect(m_session, m_signals.at(Activated));
    m_signals.at(Activated) = 0;
  }

  if (m_signals.at(Deactivated)) {
    g_signal_handler_disconnect(m_session, m_signals.at(Deactivated));
    m_signals.at(Deactivated) = 0;
  }

  if (m_signals.at(ShortcutsChanged)) {
    g_signal_handler_disconnect(m_session, m_signals.at(ShortcutsChanged));
    m_signals.at(ShortcutsChanged) = 0;
  }

  xdp_global_shortcuts_session_close(m_session);
  g_object_unref(m_session);
  m_session = nullptr;
}

void PortalGlobalShortcuts::setupSession(XdpGlobalShortcutsSession *session)
{
  auto parentSession = xdp_global_shortcuts_session_get_session(session);

  using enum Signal;
  m_signals.at(SessionClosed) = g_signal_connect(G_OBJECT(parentSession), "closed", G_CALLBACK(sessionClosed), this);
  m_signals.at(Activated) = g_signal_connect(G_OBJECT(session), "activated", G_CALLBACK(activated), this);
  m_signals.at(Deactivated) = g_signal_connect(G_OBJECT(session), "deactivated", G_CALLBACK(deactivated), this);
  m_signals.at(ShortcutsChanged) =
      g_signal_connect(G_OBJECT(session), "shortcuts-changed", G_CALLBACK(shortcutsChanged), this);

  bindShortcuts();
}

void PortalGlobalShortcuts::bindShortcuts()
{
  const auto bindings = bindingsSnapshot();
  g_autoptr(GArray) shortcuts = nullptr;

  shortcuts = g_array_new(FALSE, FALSE, sizeof(XdpGlobalShortcut));

  for (const auto &binding : bindings) {
    XdpGlobalShortcut shortcut = {
        .shortcut_id = binding.shortcutId.c_str(),
        .description = binding.description.c_str(),
        .preferred_trigger = binding.preferredTrigger.empty() ? nullptr : binding.preferredTrigger.c_str(),
    };
    g_array_append_val(shortcuts, shortcut);
  }

  xdp_global_shortcuts_session_bind_shortcuts(
      m_session, shortcuts,
      nullptr, // parent window
      nullptr, // cancellable
      [](GObject *object, GAsyncResult *res, gpointer data) {
        static_cast<PortalGlobalShortcuts *>(data)->handleBindShortcuts(object, res);
      },
      this
  );
}

void PortalGlobalShortcuts::handleInitSession(GObject *object, GAsyncResult *res)
{
  g_autoptr(GError) error = nullptr;

  LOG_DEBUG("portal global shortcuts session initialized");

  auto session = xdp_portal_create_global_shortcuts_session_finish(XDP_PORTAL(object), res, &error);
  if (!session) {
    LOG_ERR("failed to initialize global shortcuts session, quitting: %s", error->message);
    g_main_loop_quit(m_glibMainLoop);
    m_events->addEvent(Event(EventTypes::Quit));
    return;
  }

  m_session = session;

  setupSession(session);
}

void PortalGlobalShortcuts::handleBindShortcuts(GObject *object, GAsyncResult *res)
{
  g_autoptr(GError) error = nullptr;
  g_autoptr(GArray) shortcuts = nullptr;

  if (XDP_GLOBAL_SHORTCUTS_SESSION(object) != m_session) {
    return;
  }

  shortcuts = xdp_global_shortcuts_session_bind_shortcuts_finish(m_session, res, &error);
  if (!shortcuts) {
    LOG_ERR("failed to bind global shortcuts: %s", error->message);
    return;
  }

  LOG_DEBUG("bound %u global shortcuts", shortcuts->len);

  for (guint i = 0; i < shortcuts->len; i++) {
    auto *shortcut = &g_array_index(shortcuts, XdpGlobalShortcutAssigned, i);
    LOG_DEBUG("global shortcut %s assigned to %s", shortcut->shortcut_id, shortcut->trigger_description);
  }
  // TODO: EIScreen has to be notified about the new hotkeys
  // and they have to be changed in the config
}

void PortalGlobalShortcuts::handleSessionClosed(XdpSession *)
{
  LOG_ERR("portal global shortcuts session was closed by system, exiting");
  closeSession();

  g_main_loop_quit(m_glibMainLoop);
  m_events->addEvent(Event(EventTypes::Quit));
}

void PortalGlobalShortcuts::handleActivated(
    XdpGlobalShortcutsSession *, const char *shortcutId, const guint64 timestamp, const GVariant *
)
{
  const auto hotKeyId = hotKeyIdForShortcutId(shortcutId);
  if (hotKeyId == 0) {
    LOG_WARN("unknown global shortcut activated: %s", shortcutId);
    return;
  }

  LOG_DEBUG("global shortcut activated: %s timestamp=%" PRIu64, shortcutId, timestamp);
  m_events->addEvent(Event(
      EventTypes::PrimaryScreenHotkeyDown, m_screen->getEventTarget(), IPrimaryScreen::HotKeyInfo::alloc(hotKeyId)
  ));
}

void PortalGlobalShortcuts::handleDeactivated(
    XdpGlobalShortcutsSession *, const char *shortcutId, const guint64 timestamp, const GVariant *
)
{
  const auto hotKeyId = hotKeyIdForShortcutId(shortcutId);
  if (hotKeyId == 0) {
    LOG_WARN("unknown global shortcut deactivated: %s", shortcutId);
    return;
  }

  LOG_DEBUG("global shortcut deactivated: %s timestamp=%" PRIu64, shortcutId, timestamp);
  m_events->addEvent(
      Event(EventTypes::PrimaryScreenHotkeyUp, m_screen->getEventTarget(), IPrimaryScreen::HotKeyInfo::alloc(hotKeyId))
  );
}

void PortalGlobalShortcuts::handleShortcutsChanged(XdpGlobalShortcutsSession *session, const GVariant *shortcuts)
{
  if (session != m_session) {
    return;
  }

  if (shortcuts == nullptr) {
    LOG_WARN("global shortcuts changed without shortcuts data");
    return;
  }

  LOG_DEBUG("global shortcuts changed");

  GVariantIter iter;
  const char *shortcutId = nullptr;
  GVariant *properties = nullptr;

  g_variant_iter_init(&iter, const_cast<GVariant *>(shortcuts));

  while (g_variant_iter_next(&iter, "(&s@a{sv})", &shortcutId, &properties)) {
    g_autoptr(GVariant) propertiesOwned = properties;
    g_autofree char *triggerDescription = nullptr;

    if (!g_variant_lookup(propertiesOwned, "trigger_description", "s", &triggerDescription)) {
      LOG_DEBUG("global shortcut %s changed without trigger_description", shortcutId);
      continue;
    }

    LOG_DEBUG("global shortcut %s changed to %s", shortcutId, triggerDescription);

    // TODO: EIScreen has to be notified about the new hotkeys
    // and they have to be changed in the config
  }
}

void PortalGlobalShortcuts::glibThread(const void *)
{
  auto context = g_main_loop_get_context(m_glibMainLoop);

  LOG_DEBUG("global shortcuts glib thread running");

  while (g_main_loop_is_running(m_glibMainLoop)) {
    Thread::testCancel();
    g_main_context_iteration(context, true);
  }

  LOG_DEBUG("global shortcuts glib thread shutting down");
}

// see https://specifications.freedesktop.org/shortcuts/latest/ on how to construct
// a preferred trigger string for a key combination
bool PortalGlobalShortcuts::isValidPreferredTriggerIdentifier(const std::string &name)
{
  if (name.empty()) {
    return false;
  }

  return std::ranges::all_of(name, [](unsigned char c) { return std::isalnum(c) || c == '_'; });
}

std::optional<xkb_keysym_t> PortalGlobalShortcuts::deskflowKeyToXkbKeysym(KeyID key)
{
  if (key == kKeyNone) {
    return std::nullopt;
  }

  // KeyTypes.h explicitly excludes
  // 0xE000..0xE0FF from the "X11 keysym - 0x1000" rule.
  if (key >= 0xE000 && key <= 0xE0FF) {
    return std::nullopt;
  }

  // Deskflow control/function keys. KeyTypes.h says these
  // are equal to the corresponding X11 keysym - 0x1000.
  if (key >= 0xE100 && key <= 0xEFFF) {
    return static_cast<xkb_keysym_t>(key + 0x1000);
  }

  // Remaining values are normal UTF-32/Unicode codepoints.
  if (key < 0xE000) {
    const auto keysym = xkb_utf32_to_keysym(key);
    if (keysym == XKB_KEY_NoSymbol) {
      return std::nullopt;
    }

    return keysym;
  }

  return std::nullopt;
}

std::string PortalGlobalShortcuts::keyNameForPreferredTrigger(KeyID key)
{
  const auto keysym = deskflowKeyToXkbKeysym(key);
  if (!keysym) {
    return {};
  }

  char name[128] = {};
  const int len = xkb_keysym_get_name(*keysym, name, sizeof(name));
  if (len <= 0 || static_cast<std::size_t>(len) >= sizeof(name)) {
    return {};
  }

  std::string result{name};
  if (!isValidPreferredTriggerIdentifier(result)) {
    return {};
  }

  return result;
}

std::string PortalGlobalShortcuts::makePreferredTrigger(KeyID key, KeyModifierMask mask)
{
  constexpr KeyModifierMask supportedMask = KeyModifierShift | KeyModifierControl | KeyModifierAlt | KeyModifierMeta |
                                            KeyModifierSuper | KeyModifierAltGr | KeyModifierLevel5Lock |
                                            KeyModifierCapsLock | KeyModifierNumLock | KeyModifierScrollLock;

  if ((mask & ~supportedMask) != 0) {
    return {};
  }

  const auto keyName = keyNameForPreferredTrigger(key);
  if (keyName.empty()) {
    return {};
  }

  std::vector<std::string> parts;

  if (mask & KeyModifierShift) {
    parts.emplace_back("SHIFT");
  }
  if (mask & KeyModifierControl) {
    parts.emplace_back("CTRL");
  }
  if (mask & KeyModifierAlt) {
    parts.emplace_back("ALT");
  }
  if (mask & (KeyModifierMeta | KeyModifierSuper)) {
    parts.emplace_back("LOGO");
  }
  if (mask & KeyModifierAltGr) {
    parts.emplace_back("LEVEL3");
  }
  if (mask & KeyModifierLevel5Lock) {
    parts.emplace_back("LEVEL5");
  }
  if (mask & KeyModifierCapsLock) {
    parts.emplace_back("CAPS");
  }
  if (mask & KeyModifierNumLock) {
    parts.emplace_back("NUM");
  }
  if (mask & KeyModifierScrollLock) {
    parts.emplace_back("SCROLL");
  }

  parts.emplace_back(keyName);

  std::string trigger;
  for (std::size_t i = 0; i < parts.size(); ++i) {
    if (i != 0) {
      trigger += "+";
    }
    trigger += parts[i];
  }

  return trigger;
}

std::string PortalGlobalShortcuts::makeShortcutId(KeyID key, KeyModifierMask mask)
{
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "deskflow-hotkey-%08x-%08x", mask, key);
  return buffer;
}

} // namespace deskflow
