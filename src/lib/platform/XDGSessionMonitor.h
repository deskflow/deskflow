/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <gio/gio.h>

#include <atomic>
#include <functional>
#include <string>

namespace deskflow {

/**
 * @brief Tracks whether the desktop is unlocked and awake, using logind.
 *
 * Portal sessions requested while the screen is locked or the system is
 * suspending cannot be validated by the compositor, which then falls back to
 * a permission prompt on unlock. Callers check isReady() before requesting a
 * session and are called back once the desktop can serve one again.
 *
 * GDBus delivers the signals to the main context that is the thread default at
 * construction, so construct this from a callback already running on the
 * context that should receive them; a Qt worker thread pushes a private context
 * that nothing else iterates. Without a system bus or a logind session it
 * reports ready, so nothing waits.
 *
 * A sandboxed build is such a case and is meant to be: Flathub rejects the
 * system bus permission this needs, so do not add it back to the Flatpak
 * manifests. Those builds keep the portal recovery but not the waiting.
 */
class XDGSessionMonitor
{
public:
  using ReadyCallback = std::function<void()>;

  /**
   * @param onReady Called when the desktop becomes unlocked and awake after
   *                having been locked or asleep.
   */
  explicit XDGSessionMonitor(ReadyCallback onReady);
  ~XDGSessionMonitor();

  XDGSessionMonitor(const XDGSessionMonitor &) = delete;
  XDGSessionMonitor &operator=(const XDGSessionMonitor &) = delete;

  /**
   * @return true when the desktop is unlocked and not preparing for sleep, or
   *         when the state cannot be determined.
   */
  bool isReady() const;

private:
  void findSession();
  bool readLockedHint() const;
  void setLocked(bool locked);
  void setSleeping(bool sleeping);
  void notifyIfReady();

  static void onPrepareForSleep(
      GDBusConnection *connection, const gchar *sender, const gchar *path, const gchar *interface, const gchar *signal,
      GVariant *parameters, gpointer data
  );
  static void onSessionPropertiesChanged(
      GDBusConnection *connection, const gchar *sender, const gchar *path, const gchar *interface, const gchar *signal,
      GVariant *parameters, gpointer data
  );

  GDBusConnection *m_bus = nullptr;
  std::string m_sessionPath;
  guint m_sleepSubscription = 0;
  guint m_lockSubscription = 0;
  std::atomic<bool> m_locked{false};
  std::atomic<bool> m_sleeping{false};
  std::atomic<bool> m_wasReady{true};
  ReadyCallback m_onReady;
};

} // namespace deskflow
