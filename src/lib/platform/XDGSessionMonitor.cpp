/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/XDGSessionMonitor.h"

#include "base/Log.h"

namespace deskflow {

namespace {

const auto kLogindName = "org.freedesktop.login1";
const auto kLogindPath = "/org/freedesktop/login1";
const auto kManagerInterface = "org.freedesktop.login1.Manager";
const auto kSessionInterface = "org.freedesktop.login1.Session";
const auto kPropertiesInterface = "org.freedesktop.DBus.Properties";
const auto kAutoSessionPath = "/org/freedesktop/login1/session/auto";

GVariant *getProperty(GDBusConnection *bus, const std::string &path, const char *name)
{
  g_autoptr(GError) error = nullptr;
  g_autoptr(GVariant) reply = g_dbus_connection_call_sync(
      bus, kLogindName, path.c_str(), kPropertiesInterface, "Get", g_variant_new("(ss)", kSessionInterface, name),
      G_VARIANT_TYPE("(v)"), G_DBUS_CALL_FLAGS_NONE, -1, nullptr, &error
  );
  if (!reply) {
    LOG_DEBUG("logind session property %s unavailable: %s", name, error->message);
    return nullptr;
  }

  GVariant *value = nullptr;
  g_variant_get(reply, "(v)", &value);
  return value;
}

} // namespace

XDGSessionMonitor::XDGSessionMonitor(ReadyCallback onReady) : m_onReady{std::move(onReady)}
{
  g_autoptr(GError) error = nullptr;
  m_bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error);
  if (!m_bus) {
    LOG_DEBUG("no system bus, portal sessions will not wait for the desktop to unlock: %s", error->message);
    return;
  }

  m_sleepSubscription = g_dbus_connection_signal_subscribe(
      m_bus, kLogindName, kManagerInterface, "PrepareForSleep", kLogindPath, nullptr, G_DBUS_SIGNAL_FLAGS_NONE,
      onPrepareForSleep, this, nullptr
  );

  findSession();
  if (m_sessionPath.empty()) {
    LOG_DEBUG("no logind session for this desktop, portal sessions will not wait for the screen to unlock");
    return;
  }

  m_lockSubscription = g_dbus_connection_signal_subscribe(
      m_bus, kLogindName, kPropertiesInterface, "PropertiesChanged", m_sessionPath.c_str(), kSessionInterface,
      G_DBUS_SIGNAL_FLAGS_NONE, onSessionPropertiesChanged, this, nullptr
  );
  m_locked = readLockedHint();
  m_wasReady = isReady();
  LOG_DEBUG("watching logind session %s for lock state, locked=%d", m_sessionPath.c_str(), m_locked.load());
}

XDGSessionMonitor::~XDGSessionMonitor()
{
  if (m_bus) {
    if (m_sleepSubscription) {
      g_dbus_connection_signal_unsubscribe(m_bus, m_sleepSubscription);
    }
    if (m_lockSubscription) {
      g_dbus_connection_signal_unsubscribe(m_bus, m_lockSubscription);
    }
    g_object_unref(m_bus);
  }
}

bool XDGSessionMonitor::isReady() const
{
  return !m_locked && !m_sleeping;
}

// Desktops launch apps in systemd scopes outside the login session cgroup, so
// GetSessionByPID fails and XDG_SESSION_ID is unset. The "auto" alias resolves to the
// caller's session, or the user's display session, but signals are emitted on the real
// path, so resolve that once here.
void XDGSessionMonitor::findSession()
{
  g_autoptr(GVariant) id = getProperty(m_bus, kAutoSessionPath, "Id");
  if (!id) {
    return;
  }

  g_autoptr(GError) error = nullptr;
  g_autoptr(GVariant) reply = g_dbus_connection_call_sync(
      m_bus, kLogindName, kLogindPath, kManagerInterface, "GetSession",
      g_variant_new("(s)", g_variant_get_string(id, nullptr)), G_VARIANT_TYPE("(o)"), G_DBUS_CALL_FLAGS_NONE, -1,
      nullptr, &error
  );
  if (!reply) {
    LOG_DEBUG("cannot resolve the logind session path: %s", error->message);
    return;
  }

  const gchar *path = nullptr;
  g_variant_get(reply, "(&o)", &path);
  m_sessionPath = path;
}

bool XDGSessionMonitor::readLockedHint() const
{
  g_autoptr(GVariant) locked = getProperty(m_bus, m_sessionPath, "LockedHint");
  return locked && g_variant_get_boolean(locked);
}

void XDGSessionMonitor::setLocked(bool locked)
{
  if (m_locked.exchange(locked) != locked) {
    LOG_DEBUG("desktop %s", locked ? "locked" : "unlocked");
    notifyIfReady();
  }
}

void XDGSessionMonitor::setSleeping(bool sleeping)
{
  if (m_sleeping.exchange(sleeping) != sleeping) {
    LOG_DEBUG("system %s", sleeping ? "preparing for sleep" : "resumed");
    notifyIfReady();
  }
}

void XDGSessionMonitor::notifyIfReady()
{
  const bool ready = isReady();
  if (m_wasReady.exchange(ready) || !ready) {
    return;
  }
  if (m_onReady) {
    m_onReady();
  }
}

void XDGSessionMonitor::onPrepareForSleep(
    GDBusConnection *, const gchar *, const gchar *, const gchar *, const gchar *, GVariant *parameters, gpointer data
)
{
  gboolean sleeping = FALSE;
  g_variant_get(parameters, "(b)", &sleeping);

  auto self = static_cast<XDGSessionMonitor *>(data);
  if (!sleeping && !self->m_sessionPath.empty()) {
    // the lock screen usually engages while asleep, before any change signal reaches us
    self->m_locked = self->readLockedHint();
  }
  self->setSleeping(sleeping);
}

void XDGSessionMonitor::onSessionPropertiesChanged(
    GDBusConnection *, const gchar *, const gchar *, const gchar *, const gchar *, GVariant *parameters, gpointer data
)
{
  g_autoptr(GVariant) changed = g_variant_get_child_value(parameters, 1);
  g_autoptr(GVariant) locked = g_variant_lookup_value(changed, "LockedHint", G_VARIANT_TYPE_BOOLEAN);
  if (locked) {
    static_cast<XDGSessionMonitor *>(data)->setLocked(g_variant_get_boolean(locked));
  }
}

} // namespace deskflow
