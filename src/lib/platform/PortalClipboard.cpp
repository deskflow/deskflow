/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/PortalClipboard.h"

#include "base/Log.h"

#include <common/Settings.h>

#include <cstring>

#include <QDateTime>

namespace {

// MIME types for different clipboard formats
constexpr const char *kMimeTypeText = "text/plain;charset=utf-8";
constexpr const char *kMimeTypeTextPlain = "text/plain";
constexpr const char *kMimeTypeHtml = "text/html";
constexpr const char *kMimeTypeBmp = "image/bmp";

// Cache validity duration in milliseconds
constexpr int kCacheValidityMs = 100;

// Portal availability check result (cached)
std::atomic<bool> s_portalClipboardAvailable{false};
std::atomic<bool> s_portalClipboardChecked{false};

} // namespace

PortalClipboard::PortalClipboard(ClipboardID id)
    : m_id(id),
      m_useClipboard(id == kClipboardClipboard)
{
  // Initialize cached data
  for (int i = 0; i < static_cast<int>(Format::TotalFormats); ++i) {
    m_cachedAvailable[i] = false;
  }

  // Create portal connection
  m_portal = xdp_portal_new();

  LOG_DEBUG("portal clipboard created for %s selection", m_useClipboard ? "clipboard" : "primary");
}

PortalClipboard::~PortalClipboard()
{
  stopMonitoring();

  if (m_portal) {
    g_object_unref(m_portal);
    m_portal = nullptr;
  }

  LOG_DEBUG("portal clipboard destroyed");
}

ClipboardID PortalClipboard::getID() const
{
  return m_id;
}

bool PortalClipboard::isAvailable()
{
  // Check cached result first
  if (s_portalClipboardChecked.load()) {
    return s_portalClipboardAvailable.load();
  }

  // Check if portal supports clipboard by querying the session bus
  g_autoptr(GDBusConnection) bus = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
  if (!bus) {
    LOG_DEBUG("portal clipboard not available: no session bus");
    s_portalClipboardChecked = true;
    s_portalClipboardAvailable = false;
    return false;
  }

  // Check for the Clipboard interface on the portal
  g_autoptr(GError) error = nullptr;
  g_autoptr(GVariant) version =
      g_dbus_connection_call_sync(bus, "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
                                  "org.freedesktop.DBus.Properties", "Get",
                                  g_variant_new("(ss)", "org.freedesktop.portal.Clipboard", "version"), nullptr,
                                  G_DBUS_CALL_FLAGS_NONE, 5000, nullptr, &error);

  if (error || !version) {
    LOG_DEBUG("portal clipboard not available: no clipboard portal support (%s)", error ? error->message : "unknown");
    s_portalClipboardChecked = true;
    s_portalClipboardAvailable = false;
    return false;
  }

  // Cache the positive result
  s_portalClipboardChecked = true;
  s_portalClipboardAvailable = true;
  LOG_DEBUG("portal clipboard available");
  return true;
}

bool PortalClipboard::isEnabled()
{
  // Portal clipboard uses the same setting as wl-clipboard for now
  // When UseWlClipboard is true, we try portal first, then fall back to wl-clipboard
  return Settings::value(Settings::Core::UseWlClipboard).toBool();
}

void PortalClipboard::startMonitoring()
{
  if (!m_portal) {
    LOG_WARN("portal clipboard not initialized");
    return;
  }

  // Request clipboard access if not already done
  if (!m_hasAccess && !m_accessRequested) {
    requestClipboardAccess();
  }

  LOG_DEBUG("portal clipboard monitoring started");
}

void PortalClipboard::stopMonitoring()
{
  if (m_signalId && m_portal) {
    g_signal_handler_disconnect(m_portal, m_signalId);
    m_signalId = 0;
  }
  m_hasAccess = false;
  m_accessRequested = false;
  LOG_DEBUG("portal clipboard monitoring stopped");
}

bool PortalClipboard::hasChanged() const
{
  return m_hasChanged.load();
}

void PortalClipboard::resetChanged()
{
  m_hasChanged = false;
  std::scoped_lock<std::mutex> lock(m_cacheMutex);
  invalidateCache();
}

bool PortalClipboard::empty()
{
  if (!m_open) {
    return false;
  }

  // Clear the clipboard
  std::scoped_lock<std::mutex> lock(m_cacheMutex);
  invalidateCache();
  m_owned = true;

  return true;
}

void PortalClipboard::add(Format format, const std::string &data)
{
  if (!m_open) {
    return;
  }

  QString mimeType = formatToMimeType(format);
  if (mimeType.isEmpty()) {
    LOG_WARN("portal clipboard: unsupported format: %d", static_cast<int>(format));
    return;
  }

  // Cache the data locally
  {
    std::scoped_lock<std::mutex> lock(m_cacheMutex);
    m_cachedData[static_cast<int>(format)] = data;
    m_cachedAvailable[static_cast<int>(format)] = true;
    m_cached = true;
    m_cachedTime = getCurrentTime();
  }

  // Note: Setting clipboard content via portal requires xdp_portal_set_clipboard()
  // which is available in libportal 0.9.1+ but may not be implemented by all backends
  LOG_DEBUG("portal clipboard: added %zu bytes for format %s", data.size(), mimeType.toUtf8().constData());
}

bool PortalClipboard::open(Time time) const
{
  if (m_open) {
    LOG_DEBUG("portal clipboard: already open");
    return false;
  }

  m_open = true;
  m_time = time;

  return true;
}

void PortalClipboard::close() const
{
  if (!m_open) {
    return;
  }

  LOG_DEBUG("portal clipboard: closed");
  m_open = false;
}

IClipboard::Time PortalClipboard::getTime() const
{
  return m_time;
}

bool PortalClipboard::has(Format format) const
{
  if (!m_open) {
    return false;
  }

  std::scoped_lock<std::mutex> lock(m_cacheMutex);

  // Check cache validity
  Time currentTime = getCurrentTime();
  if (m_cached && (currentTime - m_cachedTime) < kCacheValidityMs) {
    return m_cachedAvailable[static_cast<int>(format)];
  }

  // Cache miss - we don't have synchronous access to portal clipboard
  // The data should be populated by the selection-owner-changed signal
  return false;
}

std::string PortalClipboard::get(Format format) const
{
  if (!m_open) {
    return std::string();
  }

  std::scoped_lock<std::mutex> lock(m_cacheMutex);

  // Return cached data if available and valid
  if (m_cached && m_cachedAvailable[static_cast<int>(format)] && !m_cachedData[static_cast<int>(format)].empty()) {
    return m_cachedData[static_cast<int>(format)];
  }

  // Cache miss - the data should have been populated by signals
  // For portal clipboard, we can't synchronously fetch data
  return std::string();
}

QString PortalClipboard::formatToMimeType(Format format) const
{
  switch (format) {
  case Format::Text:
    return QString::fromUtf8(kMimeTypeText);
  case Format::HTML:
    return QString::fromUtf8(kMimeTypeHtml);
  case Format::Bitmap:
    return QString::fromUtf8(kMimeTypeBmp);
  default:
    return {};
  }
}

IClipboard::Format PortalClipboard::mimeTypeToFormat(const QString &mimeType) const
{
  if (mimeType.isEmpty())
    return Format::Text;

  const auto mimeTypeUtf8 = mimeType.toUtf8();
  const char *mime = mimeTypeUtf8.constData();

  if (strcmp(mime, kMimeTypeText) == 0 || strcmp(mime, kMimeTypeTextPlain) == 0) {
    return Format::Text;
  }
  if (strcmp(mime, kMimeTypeHtml) == 0 || strstr(mime, "text/html") != nullptr) {
    return Format::HTML;
  }
  if (strcmp(mime, kMimeTypeBmp) == 0) {
    return Format::Bitmap;
  }

  // Default to text for unknown types
  return Format::Text;
}

void PortalClipboard::requestClipboardAccess()
{
  if (!m_portal) {
    LOG_WARN("portal clipboard: no portal connection");
    return;
  }

  m_accessRequested = true;

// Check if libportal has clipboard access API
// This is only available in libportal 0.9.1+ with clipboard support
#if defined(XDP_CHECK_VERSION) && XDP_CHECK_VERSION(0, 9, 1)
  // Request clipboard access from the portal
  xdp_portal_access_clipboard(m_portal, nullptr, // cancellable
                               onAccessResponse, this);
#else
  // Clipboard access API not available in this libportal version
  LOG_DEBUG("portal clipboard: clipboard access API not available (libportal < 0.9.1)");
  m_hasAccess = false;
#endif
}

void PortalClipboard::handleAccessResponse(GObject *object, GAsyncResult *res)
{
#if defined(XDP_CHECK_VERSION) && XDP_CHECK_VERSION(0, 9, 1)
  g_autoptr(GError) error = nullptr;

  gboolean success = xdp_portal_access_clipboard_finish(XDP_PORTAL(object), res, &error);

  if (error) {
    LOG_WARN("portal clipboard access denied: %s", error->message);
    m_hasAccess = false;
    return;
  }

  m_hasAccess = success;
  LOG_DEBUG("portal clipboard access %s", success ? "granted" : "denied");

  // Connect to selection owner changed signal if we have access
  if (m_hasAccess && !m_signalId) {
    m_signalId = g_signal_connect(G_OBJECT(m_portal), "selection-owner-changed",
                                  G_CALLBACK(onSelectionOwnerChanged), this);
  }
#else
  std::ignore = object;
  std::ignore = res;
  m_hasAccess = false;
#endif
}

void PortalClipboard::onAccessResponse(GObject *object, GAsyncResult *res, gpointer data)
{
  auto *self = static_cast<PortalClipboard *>(data);
  self->handleAccessResponse(object, res);
}

void PortalClipboard::handleSelectionOwnerChanged(const char **mimeTypes)
{
  LOG_DEBUG("portal clipboard: selection owner changed");

  // Invalidate cache since content changed
  {
    std::scoped_lock<std::mutex> lock(m_cacheMutex);
    invalidateCache();
  }

  // Mark as changed
  m_hasChanged = true;

  // Update available formats based on mime types
  if (mimeTypes) {
    std::scoped_lock<std::mutex> lock(m_cacheMutex);
    for (int i = 0; i < static_cast<int>(Format::TotalFormats); ++i) {
      m_cachedAvailable[i] = false;
    }

    for (const char **mime = mimeTypes; *mime; ++mime) {
      QString mimeType = QString::fromUtf8(*mime);
      Format format = mimeTypeToFormat(mimeType);
      m_cachedAvailable[static_cast<int>(format)] = true;
    }
  }
}

void PortalClipboard::onSelectionOwnerChanged(XdpPortal * /*portal*/, const char **mimeTypes, gpointer data)
{
  auto *self = static_cast<PortalClipboard *>(data);
  self->handleSelectionOwnerChanged(mimeTypes);
}

IClipboard::Time PortalClipboard::getCurrentTime() const
{
  return static_cast<Time>(QDateTime::currentMSecsSinceEpoch());
}

void PortalClipboard::invalidateCache()
{
  m_cached = false;
  m_cachedTime = 0;
  for (int i = 0; i < static_cast<int>(Format::TotalFormats); ++i) {
    m_cachedData[i].clear();
    m_cachedAvailable[i] = false;
  }
}
