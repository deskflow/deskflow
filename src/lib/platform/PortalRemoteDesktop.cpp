/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-FileCopyrightText: (C) 2022 Red Hat, Inc.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/PortalRemoteDesktop.h"
#include "base/Log.h"
#include "base/TMethodJob.h"
#include "common/Settings.h"
#include <unistd.h>
#include <fcntl.h>

#include <cerrno>
#include <cstring>
#include <vector>
#include <gio/gunixfdlist.h>

namespace deskflow {

// ---------------------------------------------------------------------------
// MIME type mapping — IClipboard::Format ↔ portal MIME types
// ---------------------------------------------------------------------------

struct MimeMapping
{
  IClipboard::Format format;
  std::vector<std::string> mimeTypes;
};

// First MIME type in each list is preferred (offered first in SetSelection).
// When reading (SelectionRead), we try each MIME type in order until one
// matches what the clipboard owner offers.
static const std::vector<MimeMapping> kMimeMappings = {
    {IClipboard::Format::Text, {"text/plain;charset=utf-8", "text/plain", "UTF8_STRING", "STRING"}},
    {IClipboard::Format::HTML, {"text/html", "text/html;charset=utf-8"}},
    {IClipboard::Format::Bitmap, {"image/png", "image/bmp", "image/x-bmp"}},
};

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

PortalRemoteDesktop::PortalRemoteDesktop(EiScreen *screen, IEventQueue *events, bool clipboardOnly)
    : m_screen{screen},
      m_events{events},
      m_portal{xdp_portal_new()},
      m_clipboardOnly{clipboardOnly}
{
  m_glibMainLoop = g_main_loop_new(nullptr, true);

  auto tMethodJob = new TMethodJob<PortalRemoteDesktop>(this, &PortalRemoteDesktop::glibThread);
  m_glibThread = new Thread(tMethodJob);

  reconnect(0);
}

PortalRemoteDesktop::~PortalRemoteDesktop()
{
  if (g_main_loop_is_running(m_glibMainLoop))
    g_main_loop_quit(m_glibMainLoop);

  if (m_glibThread != nullptr) {
    m_glibThread->cancel();
    m_glibThread->wait();
    delete m_glibThread;
    m_glibThread = nullptr;

    g_main_loop_unref(m_glibMainLoop);
    m_glibMainLoop = nullptr;
  }

  cleanupClipboard();

  if (m_sessionSignalId)
    g_signal_handler_disconnect(m_session, m_sessionSignalId);
  if (m_session)
    g_object_unref(m_session);
  g_object_unref(m_portal);

  free(m_sessionRestoreToken);
}

// ---------------------------------------------------------------------------
// GLib main loop / reconnection
// ---------------------------------------------------------------------------

gboolean PortalRemoteDesktop::timeoutHandler() const
{
  return true; // keep re-triggering
}

void PortalRemoteDesktop::reconnect(unsigned int timeout)
{
  auto initCallback = [](gpointer data) { return static_cast<PortalRemoteDesktop *>(data)->initSession(); };

  if (timeout > 0)
    g_timeout_add(timeout, initCallback, this);
  else
    g_idle_add(initCallback, this);
}

void PortalRemoteDesktop::glibThread(const void *)
{
  auto context = g_main_loop_get_context(m_glibMainLoop);

  while (g_main_loop_is_running(m_glibMainLoop)) {
    Thread::testCancel();
    g_main_context_iteration(context, true);
  }
}

// ---------------------------------------------------------------------------
// Session lifecycle
// ---------------------------------------------------------------------------

void PortalRemoteDesktop::handleSessionClosed(XdpSession *session)
{
  LOG_ERR("portal remote desktop session was closed, reconnecting");
  cleanupClipboard();
  g_signal_handler_disconnect(session, m_sessionSignalId);
  m_sessionSignalId = 0;

  if (!m_clipboardOnly) {
    m_events->addEvent(Event(EventTypes::EISessionClosed, m_screen->getEventTarget()));
  }

  // gcc warning "Suspicious usage of 'sizeof(A*)'" can be ignored
  g_clear_object(&m_session);

  reconnect(1000);
}

void PortalRemoteDesktop::handleSessionStarted(GObject *object, GAsyncResult *res)
{
  g_autoptr(GError) error = nullptr;
  auto session = XDP_SESSION(object);
  if (!xdp_session_start_finish(session, res, &error)) {
    LOG_ERR("failed to start portal remote desktop session, quitting: %s", error->message);
    g_main_loop_quit(m_glibMainLoop);
    m_events->addEvent(Event(EventTypes::Quit));
    return;
  }

  m_sessionRestoreToken = xdp_session_get_restore_token(session);
  if (m_sessionRestoreToken) {
    Settings::setValue(Settings::Client::XdpRestoreToken, QString(m_sessionRestoreToken));
  }

  LOG_NOTE("clipboard: session started, clipboardEnabled=%d, clipboardOnly=%d", m_clipboardEnabled.load(), m_clipboardOnly);

  if (!m_clipboardOnly) {
    // ConnectToEIS requires version 2 of the xdg-desktop-portal (and the same
    // version in the impl.portal), i.e. you'll need an updated compositor on
    // top of everything...
    auto fd = -1;
    fd = xdp_session_connect_to_eis(session, &error);
    if (fd < 0) {
      g_main_loop_quit(m_glibMainLoop);
      m_events->addEvent(Event(EventTypes::Quit));
      return;
    }

    // Socket ownership is transferred to the EiScreen
    m_events->addEvent(
        Event(EventTypes::EIConnected, m_screen->getEventTarget(), EiScreen::EiConnectInfo::alloc(fd)));
  }

}

void PortalRemoteDesktop::handleInitSession(GObject *object, GAsyncResult *res)
{
  LOG_DEBUG("portal remote desktop session initialized");
  g_autoptr(GError) error = nullptr;

  auto session = xdp_portal_create_remote_desktop_session_finish(XDP_PORTAL(object), res, &error);
  if (!session) {
    LOG_ERR("failed to initialize remote desktop session: %s", error->message);
    // This was the first attempt to connect to the RD portal - quit if that
    // fails.
    if (m_sessionIteration == 0) {
      g_main_loop_quit(m_glibMainLoop);
      m_events->addEvent(Event(EventTypes::Quit));
    } else {
      this->reconnect(1000);
    }
    return;
  }

  m_session = session;
  ++m_sessionIteration;

  // FIXME: the lambda trick doesn't work here for unknown reasons, we need
  // the static function
  m_sessionSignalId = g_signal_connect(G_OBJECT(session), "closed", G_CALLBACK(handleSessionClosedCallback), this);

  // Request clipboard BEFORE starting the session — xdg-desktop-portal requires
  // RequestClipboard to be called while the session is still in INIT state.
  // Once Start() is called and the session transitions to STARTED, it's too late.
  requestClipboard();

  LOG_DEBUG("portal remote desktop session starting");
  xdp_session_start(
      session,
      nullptr, // parent
      nullptr, // cancellable
      [](GObject *obj, GAsyncResult *res2, gpointer data) {
        static_cast<PortalRemoteDesktop *>(data)->handleSessionStarted(obj, res2);
      },
      this);
}

gboolean PortalRemoteDesktop::initSession()
{
  if (auto sessionToken = Settings::value(Settings::Client::XdpRestoreToken).toByteArray(); !sessionToken.isEmpty()) {
    free(m_sessionRestoreToken);
    m_sessionRestoreToken = strdup(sessionToken.data());
  }

  LOG_DEBUG("setting up remote desktop session with restore token %s", m_sessionRestoreToken);
  xdp_portal_create_remote_desktop_session_full(
      m_portal, static_cast<XdpDeviceType>(XDP_DEVICE_POINTER | XDP_DEVICE_KEYBOARD), XDP_OUTPUT_NONE,
      XDP_REMOTE_DESKTOP_FLAG_NONE, XDP_CURSOR_MODE_HIDDEN, XDP_PERSIST_MODE_PERSISTENT, m_sessionRestoreToken,
      nullptr, // cancellable
      [](GObject *obj, GAsyncResult *res, gpointer data) {
        static_cast<PortalRemoteDesktop *>(data)->handleInitSession(obj, res);
      },
      this);

  return false; // don't reschedule
}

// ---------------------------------------------------------------------------
// Clipboard implementation via org.freedesktop.portal.Clipboard
// ---------------------------------------------------------------------------

void PortalRemoteDesktop::requestClipboard()
{
  g_autoptr(GError) error = nullptr;

  m_dbusConnection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
  if (!m_dbusConnection) {
    LOG_WARN("clipboard: failed to get DBus session bus: %s", error->message);
    return;
  }

  // Discover session handle by enumerating DBus session paths.
  // libportal's XdpSession does not expose the session handle publicly,
  // and g_bus_get_sync() may return a connection with a different unique name
  // than libportal's internal connection, so we cannot reliably derive
  // the sender_token from our own unique name.
  //
  // Instead, introspect /org/freedesktop/portal/desktop/session/ to list all
  // sender_token subdirectories, then introspect each one to find session nodes.
  // We pick the most recently created session (last node found).
  const char *kSessionBase = "/org/freedesktop/portal/desktop/session";

  g_autoptr(GVariant) baseIntrospect = g_dbus_connection_call_sync(
      m_dbusConnection, "org.freedesktop.portal.Desktop", kSessionBase,
      "org.freedesktop.DBus.Introspectable", "Introspect", nullptr, G_VARIANT_TYPE("(s)"),
      G_DBUS_CALL_FLAGS_NONE, 5000, nullptr, &error);

  if (!baseIntrospect) {
    LOG_WARN("clipboard: failed to introspect session base path: %s", error->message);
    g_object_unref(m_dbusConnection);
    m_dbusConnection = nullptr;
    return;
  }

  // Parse XML to extract all sender_token subdirectory names
  const gchar *baseXml = nullptr;
  g_variant_get(baseIntrospect, "(&s)", &baseXml);

  std::vector<std::string> senderTokens;
  std::string baseXmlStr(baseXml);
  std::string nodeSearch = "node name=\"";
  size_t bpos = 0;
  while ((bpos = baseXmlStr.find(nodeSearch, bpos)) != std::string::npos) {
    bpos += nodeSearch.length();
    auto bend = baseXmlStr.find('"', bpos);
    if (bend != std::string::npos) {
      senderTokens.push_back(baseXmlStr.substr(bpos, bend - bpos));
      bpos = bend;
    }
  }

  if (senderTokens.empty()) {
    LOG_WARN("clipboard: no sender tokens found under %s", kSessionBase);
    g_object_unref(m_dbusConnection);
    m_dbusConnection = nullptr;
    return;
  }

  // For each sender_token, introspect to find session nodes.
  // Pick the last session node found across all sender_tokens.
  m_sessionHandle.clear();
  for (const auto &token : senderTokens) {
    std::string tokenPath = std::string(kSessionBase) + "/" + token;
    g_autoptr(GError) tokenError = nullptr;
    g_autoptr(GVariant) tokenIntrospect = g_dbus_connection_call_sync(
        m_dbusConnection, "org.freedesktop.portal.Desktop", tokenPath.c_str(),
        "org.freedesktop.DBus.Introspectable", "Introspect", nullptr, G_VARIANT_TYPE("(s)"),
        G_DBUS_CALL_FLAGS_NONE, 5000, nullptr, &tokenError);

    if (!tokenIntrospect) {
      LOG_DEBUG("clipboard: failed to introspect %s: %s", tokenPath.c_str(), tokenError->message);
      continue;
    }

    const gchar *tokenXml = nullptr;
    g_variant_get(tokenIntrospect, "(&s)", &tokenXml);

    std::string tokenXmlStr(tokenXml);
    size_t tpos = 0;
    std::string lastNode;
    while ((tpos = tokenXmlStr.find(nodeSearch, tpos)) != std::string::npos) {
      tpos += nodeSearch.length();
      auto tend = tokenXmlStr.find('"', tpos);
      if (tend != std::string::npos) {
        lastNode = tokenXmlStr.substr(tpos, tend - tpos);
        tpos = tend;
      }
    }

    if (!lastNode.empty()) {
      std::string fullPath = tokenPath + "/" + lastNode;
      // Prefer GNOME portal sessions (node starts with "portal") over GTK portal
      // sessions (node starts with "gtk"). GTK portal sessions belong to
      // xdg-desktop-portal-gtk which is a different portal backend; we need
      // the session created by xdg-desktop-portal-gnome (the RemoteDesktop
      // session used for input capture).
      if (lastNode.find("portal") == 0) {
        m_sessionHandle = fullPath;
        break; // Found a GNOME portal session, use it.
      } else if (m_sessionHandle.empty() || m_sessionHandle.find("/gtk") != std::string::npos) {
        // Only use a non-portal session as fallback if we haven't found
        // one yet or the current best is a GTK session.
        m_sessionHandle = fullPath;
      }
    }
  }

  if (m_sessionHandle.empty()) {
    LOG_WARN("clipboard: no session found under %s", kSessionBase);
    g_object_unref(m_dbusConnection);
    m_dbusConnection = nullptr;
    return;
  }

  // Log our connection unique name vs expected session owner
  const gchar *ourUniqueName = g_dbus_connection_get_unique_name(m_dbusConnection);
  LOG_NOTE("clipboard: our DBus unique name: %s", ourUniqueName ? ourUniqueName : "(null)");
  LOG_NOTE("clipboard: requesting clipboard on session %s", m_sessionHandle.c_str());

  // Call org.freedesktop.portal.Clipboard.RequestClipboard(session_handle, {})
  // Note: RequestClipboard returns no reply body (void), so reply_type should be
  // G_VARIANT_TYPE("") for an empty tuple, not nullptr (which accepts any type).
  g_autoptr(GVariant) result = g_dbus_connection_call_sync(
      m_dbusConnection, "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
      "org.freedesktop.portal.Clipboard", "RequestClipboard",
      g_variant_new("(oa{sv})", m_sessionHandle.c_str(), nullptr), G_VARIANT_TYPE("(" // empty tuple
      ")"),
      G_DBUS_CALL_FLAGS_NONE, 5000, nullptr, &error);

  if (!result) {
    LOG_WARN("clipboard: RequestClipboard failed: %s", error->message);
    g_object_unref(m_dbusConnection);
    m_dbusConnection = nullptr;
    m_sessionHandle.clear();
    return;
  }

  LOG_NOTE("clipboard: RequestClipboard succeeded (reply type: %s)", g_variant_get_type_string(result));

  // Subscribe to SelectionOwnerChanged signal
  m_selectionOwnerChangedSub = g_dbus_connection_signal_subscribe(
      m_dbusConnection, nullptr, // sender (any)
      "org.freedesktop.portal.Clipboard", "SelectionOwnerChanged", nullptr, // object_path (any)
      nullptr,                                                   // arg0
      G_DBUS_SIGNAL_FLAGS_NONE, cbSelectionOwnerChanged, this, nullptr);

  // Subscribe to SelectionTransfer signal
  m_selectionTransferSub = g_dbus_connection_signal_subscribe(
      m_dbusConnection, nullptr, // sender (any)
      "org.freedesktop.portal.Clipboard", "SelectionTransfer", nullptr, // object_path (any)
      nullptr,                                              // arg0
      G_DBUS_SIGNAL_FLAGS_NONE, cbSelectionTransfer, this, nullptr);

  m_clipboardEnabled = true;
  LOG_NOTE("clipboard: wayland clipboard sharing enabled via XDG Portal");
  LOG_NOTE("clipboard: session handle: %s", m_sessionHandle.c_str());
}

void PortalRemoteDesktop::cleanupClipboard()
{
  if (!m_dbusConnection)
    return;

  if (m_selectionOwnerChangedSub) {
    g_dbus_connection_signal_unsubscribe(m_dbusConnection, m_selectionOwnerChangedSub);
    m_selectionOwnerChangedSub = 0;
  }
  if (m_selectionTransferSub) {
    g_dbus_connection_signal_unsubscribe(m_dbusConnection, m_selectionTransferSub);
    m_selectionTransferSub = 0;
  }

  g_object_unref(m_dbusConnection);
  m_dbusConnection = nullptr;
  m_sessionHandle.clear();
  m_clipboardEnabled = false;

  std::lock_guard<std::mutex> lock(m_clipboardMutex);
  m_availableMimeTypes.clear();
}

// ---------------------------------------------------------------------------
// Signal handlers
// ---------------------------------------------------------------------------

void PortalRemoteDesktop::onSelectionOwnerChanged(
    GDBusConnection * /*connection*/, const gchar * /*sender_name*/, const gchar * /*object_path*/,
    const gchar * /*interface_name*/, const gchar * /*signal_name*/, GVariant *parameters)
{
  // Signal signature: (oa{sv})
  // session_handle, options{mime_types: as, session_is_owner: b}
  const gchar *sessionHandle = nullptr;
  g_autoptr(GVariantIter) optionsIter = nullptr;
  g_variant_get(parameters, "(&oa{sv})", &sessionHandle, &optionsIter);

  // Check this signal is for our session
  if (m_sessionHandle != sessionHandle)
    return;

  // Extract mime_types from options
  std::vector<std::string> mimeTypes;
  bool sessionIsOwner = false;

  const gchar *key = nullptr;
  GVariant *value = nullptr;
  while (g_variant_iter_loop(optionsIter, "{&sv}", &key, &value)) {
    if (g_strcmp0(key, "mime_types") == 0 && g_variant_is_of_type(value, G_VARIANT_TYPE_STRING_ARRAY)) {
      gsize nTypes = 0;
      const gchar **types = g_variant_get_strv(value, &nTypes);
      for (gsize i = 0; i < nTypes; ++i) {
        mimeTypes.emplace_back(types[i]);
      }
      g_free(types);
    } else if (g_strcmp0(key, "session_is_owner") == 0 &&
               g_variant_is_of_type(value, G_VARIANT_TYPE_BOOLEAN)) {
      sessionIsOwner = g_variant_get_boolean(value);
    }
  }

  // Log the offered MIME types for debugging
  std::string mimeList;
  for (const auto &mt : mimeTypes) {
    if (!mimeList.empty())
      mimeList += ", ";
    mimeList += mt;
  }
  LOG_DEBUG("clipboard: SelectionOwnerChanged with %zu mime types [%s], session_is_owner=%d", mimeTypes.size(),
            mimeList.c_str(), sessionIsOwner);

  // If we own the clipboard (we just set it via SetSelection), ignore.
  {
    std::lock_guard<std::mutex> lock(m_clipboardMutex);
    if (sessionIsOwner || m_weOwnClipboard) {
      // Only clear the flag when we see mime_types (the "real" owner changed event).
      if (!mimeTypes.empty()) {
        m_weOwnClipboard = false;
      }
      LOG_DEBUG("clipboard: ignoring SelectionOwnerChanged (we are the owner)");
      return;
    }
  }

  // Ignore events with no mime types (clipboard cleared/lost)
  if (mimeTypes.empty()) {
    return;
  }

  {
    std::lock_guard<std::mutex> lock(m_clipboardMutex);
    m_availableMimeTypes = std::move(mimeTypes);
  }

  // Notify Deskflow that clipboard has been grabbed by another application
  m_screen->sendClipboardEvent(EventTypes::ClipboardGrabbed, kClipboardClipboard);
}

void PortalRemoteDesktop::onSelectionTransfer(
    GDBusConnection * /*connection*/, const gchar * /*sender_name*/, const gchar * /*object_path*/,
    const gchar * /*interface_name*/, const gchar * /*signal_name*/, GVariant *parameters)
{
  // Signal signature: (osu) — session_handle, mime_type, serial
  const gchar *sessionHandle = nullptr;
  const gchar *mimeType = nullptr;
  guint32 serial = 0;
  g_variant_get(parameters, "(&o&su)", &sessionHandle, &mimeType, &serial);

  if (m_sessionHandle != sessionHandle)
    return;

  LOG_DEBUG("clipboard: SelectionTransfer requested for mime=%s serial=%u", mimeType, serial);

  // Find the matching format for this mime type
  std::string data;
  {
    std::lock_guard<std::mutex> lock(m_clipboardMutex);
    m_storedClipboard.open(0);
    for (const auto &mapping : kMimeMappings) {
      bool found = false;
      for (const auto &mt : mapping.mimeTypes) {
        if (mt == mimeType) {
          if (m_storedClipboard.has(mapping.format)) {
            data = m_storedClipboard.get(mapping.format);
          }
          found = true;
          break;
        }
      }
      if (found)
        break;
    }
    m_storedClipboard.close();
  }

  // Call SelectionWrite to get an fd to write to
  g_autoptr(GError) error = nullptr;
  g_autoptr(GUnixFDList) fdList = nullptr;
  g_autoptr(GVariant) result = g_dbus_connection_call_with_unix_fd_list_sync(
      m_dbusConnection, "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
      "org.freedesktop.portal.Clipboard", "SelectionWrite",
      g_variant_new("(ou)", m_sessionHandle.c_str(), serial), G_VARIANT_TYPE("(h)"),
      G_DBUS_CALL_FLAGS_NONE, 5000, nullptr, // in fd_list
      &fdList,                                // out fd_list
      nullptr,                                // cancellable
      &error);

  if (!result) {
    LOG_WARN("clipboard: SelectionWrite failed: %s", error->message);
    return;
  }

  gint32 fdIndex = 0;
  g_variant_get(result, "(h)", &fdIndex);
  int fd = g_unix_fd_list_get(fdList, fdIndex, &error);
  if (fd < 0) {
    LOG_WARN("clipboard: failed to get fd from SelectionWrite: %s", error->message);
    return;
  }

  // Write the clipboard data to the fd
  bool success = true;
  if (!data.empty()) {
    const char *ptr = data.c_str();
    size_t remaining = data.size();
    while (remaining > 0) {
      ssize_t written = write(fd, ptr, remaining);
      if (written < 0) {
        if (errno == EINTR)
          continue;
        LOG_WARN("clipboard: write to SelectionWrite fd failed: %s", strerror(errno));
        success = false;
        break;
      }
      ptr += written;
      remaining -= written;
    }
  }
  close(fd);

  // Notify that writing is done
  g_autoptr(GError) doneError = nullptr;
  g_autoptr(GVariant) doneResult = g_dbus_connection_call_sync(
      m_dbusConnection, "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
      "org.freedesktop.portal.Clipboard", "SelectionWriteDone",
      g_variant_new("(oub)", m_sessionHandle.c_str(), serial, success), nullptr, G_DBUS_CALL_FLAGS_NONE, 5000,
      nullptr, &doneError);

  if (!doneResult) {
    LOG_WARN("clipboard: SelectionWriteDone failed: %s", doneError->message);
  } else {
    LOG_DEBUG("clipboard: SelectionTransfer completed for mime=%s (%zu bytes)", mimeType, data.size());
  }
}

// ---------------------------------------------------------------------------
// Read clipboard data from portal for a specific MIME type
// ---------------------------------------------------------------------------

std::string PortalRemoteDesktop::readMimeTypeFromPortal(const std::string &mimeType) const
{
  g_autoptr(GError) error = nullptr;
  g_autoptr(GUnixFDList) fdList = nullptr;
  g_autoptr(GVariant) result = g_dbus_connection_call_with_unix_fd_list_sync(
      m_dbusConnection, "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
      "org.freedesktop.portal.Clipboard", "SelectionRead",
      g_variant_new("(os)", m_sessionHandle.c_str(), mimeType.c_str()), G_VARIANT_TYPE("(h)"),
      G_DBUS_CALL_FLAGS_NONE, 5000, nullptr, // in fd_list
      &fdList,                                // out fd_list
      nullptr,                                // cancellable
      &error);

  if (!result) {
    LOG_DEBUG("clipboard: SelectionRead for %s failed: %s", mimeType.c_str(), error->message);
    return {};
  }

  gint32 fdIndex = 0;
  g_variant_get(result, "(h)", &fdIndex);
  int fd = g_unix_fd_list_get(fdList, fdIndex, &error);
  if (fd < 0) {
    LOG_WARN("clipboard: failed to get fd from SelectionRead: %s", error->message);
    return {};
  }

  // Set fd to blocking mode — the portal may return a non-blocking fd
  // and read() would fail with EAGAIN if data isn't immediately available.
  int flags = fcntl(fd, F_GETFL);
  if (flags >= 0) {
    fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
  }

  // Read all data from the fd (limit to 100MB to prevent runaway reads)
  static constexpr size_t kMaxClipboardRead = 100 * 1024 * 1024;
  std::string data;
  char buf[4096];
  while (true) {
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n > 0) {
      data.append(buf, n);
      if (data.size() > kMaxClipboardRead) {
        LOG_WARN("clipboard: SelectionRead for %s exceeded size limit (%zu bytes), truncating", mimeType.c_str(),
                 data.size());
        break;
      }
    } else if (n == 0) {
      break; // EOF
    } else {
      if (errno == EINTR)
        continue;
      LOG_WARN("clipboard: read from SelectionRead fd failed: %s", strerror(errno));
      break;
    }
  }
  close(fd);

  LOG_DEBUG("clipboard: SelectionRead for %s returned %zu bytes", mimeType.c_str(), data.size());
  return data;
}

// ---------------------------------------------------------------------------
// Public clipboard API
// ---------------------------------------------------------------------------

bool PortalRemoteDesktop::getClipboard(ClipboardID id, IClipboard *clipboard) const
{
  if (!m_clipboardEnabled || id != kClipboardClipboard)
    return false;

  // Get the list of available mime types (set by SelectionOwnerChanged)
  std::vector<std::string> available;
  {
    std::lock_guard<std::mutex> lock(m_clipboardMutex);
    available = m_availableMimeTypes;
  }

  if (available.empty())
    return false;

  auto time = static_cast<IClipboard::Time>(0);
  if (!clipboard->open(time))
    return false;

  // Read clipboard data for each format.
  // For image formats, only read ONE (the most compact/preferred) to avoid
  // sending megabytes of redundant data over the network.
  bool gotAny = false;
  bool gotImage = false;
  for (const auto &mapping : kMimeMappings) {
    bool isImage = (mapping.format == IClipboard::Format::Bitmap);
    if (isImage && gotImage)
      continue;

    // Try each MIME type in preference order
    for (const auto &mime : mapping.mimeTypes) {
      // Check if this MIME type is offered by the clipboard owner
      bool offered = false;
      for (const auto &avail : available) {
        if (avail == mime) {
          offered = true;
          break;
        }
      }
      if (!offered)
        continue;

      auto data = readMimeTypeFromPortal(mime);
      if (!data.empty()) {
        clipboard->add(mapping.format, data);
        gotAny = true;
        if (isImage) {
          gotImage = true;
        }
        break; // got data for this format, move to next
      }
    }
  }

  clipboard->close();
  return gotAny;
}

bool PortalRemoteDesktop::setClipboard(ClipboardID id, const IClipboard *clipboard)
{
  if (!m_clipboardEnabled || id != kClipboardClipboard)
    return false;

  // Store clipboard data for later SelectionTransfer requests.
  {
    std::lock_guard<std::mutex> lock(m_clipboardMutex);
    if (clipboard != nullptr) {
      IClipboard::copy(&m_storedClipboard, clipboard);
    } else {
      auto time = static_cast<IClipboard::Time>(0);
      m_storedClipboard.open(time);
      m_storedClipboard.empty();
      m_storedClipboard.close();
    }
  }

  // Build the list of offered MIME types for SetSelection
  GVariantBuilder mimeBuilder;
  g_variant_builder_init(&mimeBuilder, G_VARIANT_TYPE("as"));
  {
    std::lock_guard<std::mutex> lock(m_clipboardMutex);
    m_storedClipboard.open(0);
    for (const auto &mapping : kMimeMappings) {
      if (m_storedClipboard.has(mapping.format)) {
        for (const auto &mime : mapping.mimeTypes) {
          g_variant_builder_add(&mimeBuilder, "s", mime.c_str());
        }
      }
    }
    m_storedClipboard.close();
  }

  // Build options dict with mime_types
  GVariantBuilder optionsBuilder;
  g_variant_builder_init(&optionsBuilder, G_VARIANT_TYPE("a{sv}"));
  g_variant_builder_add(&optionsBuilder, "{sv}", "mime_types", g_variant_builder_end(&mimeBuilder));

  // Mark that we own the clipboard so we ignore the subsequent SelectionOwnerChanged
  {
    std::lock_guard<std::mutex> lock(m_clipboardMutex);
    m_weOwnClipboard = true;
  }

  // Call SetSelection
  g_autoptr(GError) error = nullptr;
  g_autoptr(GVariant) result = g_dbus_connection_call_sync(
      m_dbusConnection, "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
      "org.freedesktop.portal.Clipboard", "SetSelection",
      g_variant_new("(oa{sv})", m_sessionHandle.c_str(), &optionsBuilder), nullptr, G_DBUS_CALL_FLAGS_NONE, 5000,
      nullptr, &error);

  if (!result) {
    LOG_WARN("clipboard: SetSelection failed: %s (clipboardEnabled=%d, sessionHandle=%s)", error->message, m_clipboardEnabled.load(), m_sessionHandle.c_str());
    std::lock_guard<std::mutex> lock(m_clipboardMutex);
    m_weOwnClipboard = false;
    return false;
  }

  LOG_DEBUG("clipboard: SetSelection succeeded");
  return true;
}

} // namespace deskflow
