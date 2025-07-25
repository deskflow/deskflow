/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/EiClipboard.h"
#include "base/Log.h"
#include "platform/EiClipboardMonitor.h"
#include "platform/EiClipboardNegotiator.h"
#include "platform/EiClipboardSync.h"
#include "platform/PortalClipboard.h"

#ifdef DESKFLOW_UNIT_TESTING
#undef HAVE_LIBPORTAL_CLIPBOARD
#define HAVE_LIBPORTAL_CLIPBOARD 0
#endif

#ifdef HAVE_LIBPORTAL_CLIPBOARD
#include <gio/gio.h>
#include <glib.h>
#include <libportal/inputcapture.h>
#include <libportal/portal.h>
#else
using GObject = void;
using GAsyncResult = void;
using gpointer = void *;
using XdpPortal = void;
static inline XdpPortal *xdp_portal_new()
{
  return nullptr;
}
static inline void g_object_unref(void *)
{
}
static inline GDBusConnection *g_bus_get_sync(GBusType, GCancellable *, GError **)
{
  return nullptr;
}
static inline GDBusProxy *g_dbus_proxy_new_sync(
    GDBusConnection *, GDBusProxyFlags, GDBusInterfaceInfo *, const char *, const char *, const char *, GCancellable *,
    GError **
)
{
  return nullptr;
}
#endif

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <regex>
#include <sstream>
#include <vector>

namespace deskflow {

EiClipboard::EiClipboard()
#ifndef __APPLE__
    : m_monitor(std::make_unique<EiClipboardMonitor>()),
      m_portalClipboard(nullptr),
      m_operationComplete(false),
      m_operationSuccess(false),
      m_open(false),
      m_time(0),
      m_portalAvailable(false),
      m_syncOptimizationEnabled(true),
      m_maxDataSize(kDefaultMaxDataSize),
      m_cacheTimeout(kDefaultCacheTimeout)
#else
    : m_open(false),
      m_time(0),
      m_syncOptimizationEnabled(true),
      m_maxDataSize(kDefaultMaxDataSize)
#endif
{
  // Initialize format arrays
  for (int i = 0; i < kNumFormats; ++i) {
    m_added[i] = false;
    m_data[i] = "";
#ifndef __APPLE__
    m_cacheValid[i] = false;
    m_cachedData[i] = "";
    m_cacheTimestamp[i] = std::chrono::steady_clock::time_point{};
#endif
  }

#ifndef __APPLE__
#ifdef DESKFLOW_UNIT_TESTING
  // For unit testing, explicitly enable portal to allow testing portal-related logic
  // and call initPortal.
  if (!m_portalClipboard) {
    initPortal();
  }
#else
  // Original logic for non-Apple, non-unit-testing builds
  if (deskflow::platform::kHasPortalClipboard) {
    if (!m_portalClipboard) {
      initPortal();
    }
  } else {
    m_portalAvailable = false;
    LOG_DEBUG("libportal support disabled at compile time for non-Apple platform");
  }
#endif // DESKFLOW_UNIT_TESTING
#else
  m_portalAvailable = false;
  LOG_DEBUG("compiled without libportal support, clipboard functionality disabled on Apple platform");
#endif

  // Initialize negotiator and synchronization components
  m_negotiator = std::make_shared<EiClipboardNegotiator>();
  m_sync = std::make_shared<EiClipboardSync>();
}

EiClipboard::~EiClipboard()
{
#ifndef __APPLE__
  if (m_portalClipboard) {
    // Removed g_object_unref as PortalClipboard is a Qt object, not a GObject.
    // Its destructor will handle cleanup.
  }
#endif
}

bool EiClipboard::isPortalAvailable() const
{
#ifndef __APPLE__
  return m_portalAvailable && deskflow::platform::kHasPortal;
#else
  return false;
#endif
}

bool EiClipboard::empty()
{
  if (!m_open) {
    LOG_WARN("cannot empty clipboard, not open");
    return false;
  }

#ifndef __APPLE__
  if (!isPortalAvailable()) {
    LOG_WARN("portal clipboard not available");
    return false;
  }

  // Clear local data
  for (int i = 0; i < kNumFormats; ++i) {
    m_added[i] = false;
    m_data[i] = "";
    m_cacheValid[i] = false;
    m_cachedData[i] = "";
  }

  // TODO: Implement portal clipboard clear when API is available
  LOG_DEBUG("clipboard emptied (portal clear not yet implemented)");
  return true;
#else
  // Clear local data for non-portal builds
  for (int i = 0; i < kNumFormats; ++i) {
    m_added[i] = false;
    m_data[i] = "";
  }
  return false; // Indicate clipboard functionality not available
#endif
}

void EiClipboard::add(EFormat format, const std::string &data)
{
  if (!m_open) {
    LOG_WARN("cannot add to clipboard, not open");
    return;
  }

  if (format < 0 || format >= kNumFormats) {
    LOG_WARN("invalid clipboard format: %d", format);
    return;
  }

  // Check data size limit
  if (data.size() > m_maxDataSize) {
    LOG_WARN("clipboard data too large: %zu bytes (limit: %zu)", data.size(), m_maxDataSize);
    return;
  }

  // Validate and sanitize data
  if (!validateClipboardData(format, data)) {
    LOG_WARN("clipboard data validation failed for format %d", format);
    return;
  }

  std::string sanitizedData = sanitizeClipboardData(format, data);

  LOG_DEBUG("adding %zu bytes to clipboard format %d", sanitizedData.size(), format);
  m_data[format] = sanitizedData;
  m_added[format] = true;

#ifndef __APPLE__
  // Invalidate cache for this format
  m_cacheValid[format] = false;
  m_cacheTimestamp[format] = std::chrono::steady_clock::time_point{};
#endif
}

bool EiClipboard::open(Time time) const
{
  if (m_open) {
    LOG_WARN("clipboard already open");
    return false;
  }

  LOG_DEBUG("opening clipboard");
  m_open = true;
  m_time = time;

#ifndef __APPLE__
  // Invalidate all cached data when opening
  for (int i = 0; i < kNumFormats; ++i) {
    m_cacheValid[i] = false;
  }
#endif

  return true;
}

void EiClipboard::close() const
{
  if (!m_open) {
    LOG_WARN("clipboard not open");
    return;
  }

  LOG_DEBUG("closing clipboard");
  m_open = false;

#ifndef __APPLE__
  if (isPortalAvailable()) {
    // TODO: Implement portal clipboard commit when API is available
    LOG_DEBUG("clipboard closed (portal commit not yet implemented)");
  }
#endif
}

IClipboard::Time EiClipboard::getTime() const
{
  return m_time;
}

bool EiClipboard::has(EFormat format) const
{
  if (!m_open) {
    LOG_WARN("cannot check clipboard format, not open");
    return false;
  }

  if (format < 0 || format >= kNumFormats) {
    return false;
  }

#ifndef __APPLE__
  if (isPortalAvailable()) {
    // TODO: Implement portal clipboard format checking when API is available
    LOG_DEBUG("checking clipboard format %d (portal check not yet implemented)", format);
    return m_added[format];
  }
#endif

  return m_added[format];
}

std::string EiClipboard::get(EFormat format) const
{
  if (!m_open) {
    LOG_WARN("cannot get clipboard data, not open");
    return "";
  }

  if (format < 0 || format >= kNumFormats) {
    return "";
  }

#ifndef __APPLE__
  if (isPortalAvailable()) {
    // Check cache validity
    if (m_cacheValid[format]) {
      auto now = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_cacheTimestamp[format]);

      if (elapsed < m_cacheTimeout) {
        LOG_DEBUG("returning cached clipboard data for format %d", format);
        return m_cachedData[format];
      } else {
        LOG_DEBUG("cache expired for format %d", format);
        m_cacheValid[format] = false;
      }
    }

    // TODO: Implement portal clipboard data retrieval when API is available
    LOG_DEBUG("getting clipboard data for format %d (portal get not yet implemented)", format);

    // For now, return local data and update cache
    m_cachedData[format] = m_data[format];
    m_cacheValid[format] = true;
    m_cacheTimestamp[format] = std::chrono::steady_clock::now();

    return m_data[format];
  }
#endif

  return m_data[format];
}

#ifndef __APPLE__
#ifndef DESKFLOW_UNIT_TESTING
void EiClipboard::initPortal()
{
  m_portalClipboard = std::make_unique<PortalClipboard>();
  if (!m_portalClipboard || !m_portalClipboard->initialize()) {
    m_portalClipboard = nullptr;
    return;
  }
  try {
    // Check if portal service is running
    if (checkPortalService()) {
      // Check if clipboard interface will be available
      if (checkClipboardInterface()) {
        m_portalAvailable = true;
        LOG_INFO("portal clipboard interface available and ready");

        // TODO: Connect to clipboard change signals when API is available
        // g_signal_connect(m_portal, "clipboard-changed",
        //                  G_CALLBACK(onClipboardChanged), this);
      } else {
        m_portalAvailable = false;
        LOG_INFO("portal service running but clipboard interface not available");
      }
    } else {
      m_portalAvailable = false;
      LOG_WARN("portal service not running or not accessible");
    }
  } catch (...) {
    LOG_ERR("exception while initializing portal");
    m_portalAvailable = false;
  }
}
#else  // DESKFLOW_UNIT_TESTING
void EiClipboard::initPortal()
{
  m_portalClipboard = std::make_unique<PortalClipboard>();
  if (!m_portalClipboard || !m_portalClipboard->initialize()) {
    m_portalClipboard = nullptr;
    m_portalAvailable = false; // Explicitly set to false if initialization fails
    LOG_DEBUG("Mock portal initialization failed.");
    return;
  }
  m_portalAvailable = true;
  LOG_DEBUG("Mock portal initialized for unit testing environment.");
}
#endif // DESKFLOW_UNIT_TESTING

std::string EiClipboard::formatToMimeType(EFormat format) const
{
  switch (format) {
  case kText:
    return "text/plain;charset=utf-8";
  case kBitmap:
    // Prefer PNG for lossless compression, fallback to other formats
    return "image/png";
  case kHTML:
    return "text/html;charset=utf-8";
  default:
    LOG_WARN("unknown clipboard format: %d", format);
    return "";
  }
}

std::vector<std::string> EiClipboard::formatToMimeTypes(EFormat format) const
{
  switch (format) {
  case kText:
    return {"text/plain;charset=utf-8", "text/plain", "UTF8_STRING", "STRING", "TEXT"};
  case kBitmap:
    return {"image/png", "image/jpeg", "image/bmp", "image/tiff", "image/gif"};
  case kHTML:
    return {"text/html;charset=utf-8", "text/html", "application/xhtml+xml"};
  default:
    LOG_WARN("unknown clipboard format: %d", format);
    return {};
  }
}

EiClipboard::EFormat EiClipboard::mimeTypeToFormat(const std::string &mimeType) const
{
  // Normalize MIME type (convert to lowercase, remove parameters)
  std::string normalizedType = mimeType;
  std::transform(normalizedType.begin(), normalizedType.end(), normalizedType.begin(), ::tolower);

  // Remove charset and other parameters
  size_t semicolon = normalizedType.find(';');
  if (semicolon != std::string::npos) {
    normalizedType = normalizedType.substr(0, semicolon);
  }

  // Text formats
  if (normalizedType == "text/plain" || normalizedType == "utf8_string" || normalizedType == "string" ||
      normalizedType == "text") {
    return kText;
  }

  // HTML formats
  if (normalizedType == "text/html" || normalizedType == "application/xhtml+xml") {
    return kHTML;
  }

  // Image formats
  if (normalizedType.find("image/") == 0) {
    return kBitmap;
  }

  LOG_DEBUG("unknown MIME type '%s', defaulting to text", mimeType.c_str());
  return kText; // Default fallback
}

bool EiClipboard::isSupportedMimeType(const std::string &mimeType) const
{
  // Check if we can handle this MIME type
  std::string normalizedType = mimeType;
  std::transform(normalizedType.begin(), normalizedType.end(), normalizedType.begin(), ::tolower);

  size_t semicolon = normalizedType.find(';');
  if (semicolon != std::string::npos) {
    normalizedType = normalizedType.substr(0, semicolon);
  }

  // Supported text types
  if (normalizedType == "text/plain" || normalizedType == "utf8_string" || normalizedType == "string" ||
      normalizedType == "text" || normalizedType == "text/html" || normalizedType == "application/xhtml+xml") {
    return true;
  }

  // Supported image types
  if (normalizedType == "image/png" || normalizedType == "image/jpeg" || normalizedType == "image/bmp" ||
      normalizedType == "image/tiff" || normalizedType == "image/gif") {
    return true;
  }

  return false;
}

void EiClipboard::waitForOperation() const
{
  std::unique_lock<std::mutex> lock(m_mutex);
  m_cv.wait(lock, [this] { return m_operationComplete; });
}

bool EiClipboard::checkPortalService() const
{
  // Check if the portal D-Bus service is available
#ifdef HAVE_LIBPORTAL_CLIPBOARD
  g_autoptr(GDBusConnection) connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
  if (!connection) {
    LOG_DEBUG("failed to connect to session bus");
    return false;
  }

  g_autoptr(GDBusProxy) proxy = g_dbus_proxy_new_sync(
      connection, G_DBUS_PROXY_FLAGS_NONE, nullptr, "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
      "org.freedesktop.DBus.Properties", nullptr, nullptr
  );

  if (!proxy) {
    LOG_DEBUG("portal desktop service not available");
    return false;
  }

  LOG_DEBUG("portal desktop service is available");
  return true;
#else
  return false;
#endif
}

bool EiClipboard::checkClipboardInterface() const
{
  // Check if clipboard interface is supported
  // For now, return false since the interface doesn't exist yet
  // When the interface is implemented, this should check:
  // 1. Interface introspection
  // 2. Version compatibility
  // 3. Required methods availability

  LOG_DEBUG("checking clipboard interface availability");

#ifdef HAVE_LIBPORTAL_CLIPBOARD
  // TODO: Replace with actual interface check when available
  // g_autoptr(GDBusProxy) proxy = g_dbus_proxy_new_sync(
  //     connection,
  //     G_DBUS_PROXY_FLAGS_NONE,
  //     nullptr,
  //     "org.freedesktop.portal.Desktop",
  //     "/org/freedesktop/portal/desktop",
  //     "org.freedesktop.portal.Clipboard",
  //     nullptr, nullptr);
  // return proxy != nullptr;
#endif
  return false; // Interface not yet available
}
#endif

bool EiClipboard::startMonitoring()
{
#ifndef __APPLE__
  if (m_monitor) {
    return m_monitor->startMonitoring();
  }
#endif
  return false;
}

void EiClipboard::stopMonitoring()
{
#ifndef __APPLE__
  if (deskflow::platform::kHasPortalClipboard && m_monitor && m_portalClipboard) {
    m_monitor->stopMonitoring();
    // Removed g_object_unref as PortalClipboard is a Qt object, not a GObject.
    // Its destructor will handle cleanup via std::unique_ptr.
    m_portalClipboard = nullptr;
  }
#endif
}

bool EiClipboard::isMonitoring() const
{
#ifndef __APPLE__
  if (m_monitor) {
    return m_monitor->isMonitoring();
  }
#endif
  return false;
}

void EiClipboard::setMaxDataSize(size_t maxSize)
{
  m_maxDataSize = maxSize;
  LOG_DEBUG("clipboard max data size set to %zu bytes", maxSize);
}

size_t EiClipboard::getMaxDataSize() const
{
  return m_maxDataSize;
}

void EiClipboard::clearCache()
{
#ifndef __APPLE__
  for (int i = 0; i < kNumFormats; ++i) {
    m_cacheValid[i] = false;
    m_cachedData[i] = "";
    m_cacheTimestamp[i] = std::chrono::steady_clock::time_point{};
  }
  LOG_DEBUG("clipboard cache cleared");
#endif
}

bool EiClipboard::validateClipboardData(EFormat format, const std::string &data) const
{
  // Basic validation checks
  if (data.empty()) {
    return true; // Empty data is valid
  }

  switch (format) {
  case kText: {
    // Check for valid UTF-8 encoding
    // Simple check: look for invalid byte sequences
    for (size_t i = 0; i < data.size(); ++i) {
      unsigned char c = static_cast<unsigned char>(data[i]);
      if (c > 127) {
        // Multi-byte UTF-8 character - basic validation
        if ((c & 0xE0) == 0xC0) {
          // 2-byte sequence
          if (i + 1 >= data.size() || (data[i + 1] & 0xC0) != 0x80) {
            LOG_WARN("invalid UTF-8 sequence in text data");
            return false;
          }
          i += 1;
        } else if ((c & 0xF0) == 0xE0) {
          // 3-byte sequence
          if (i + 2 >= data.size() || (data[i + 1] & 0xC0) != 0x80 || (data[i + 2] & 0xC0) != 0x80) {
            LOG_WARN("invalid UTF-8 sequence in text data");
            return false;
          }
          i += 2;
        } else if ((c & 0xF8) == 0xF0) {
          // 4-byte sequence
          if (i + 3 >= data.size() || (data[i + 1] & 0xC0) != 0x80 || (data[i + 2] & 0xC0) != 0x80 ||
              (data[i + 3] & 0xC0) != 0x80) {
            LOG_WARN("invalid UTF-8 sequence in text data");
            return false;
          }
          i += 3;
        } else {
          LOG_WARN("invalid UTF-8 byte in text data");
          return false;
        }
      }
    }
    break;
  }

  case kHTML: {
    // Basic HTML validation
    if (data.find("<script") != std::string::npos) {
      LOG_WARN("HTML data contains script tags");
      return false;
    }
    if (data.find("javascript:") != std::string::npos) {
      LOG_WARN("HTML data contains javascript URLs");
      return false;
    }
    break;
  }

  case kBitmap: {
    // Basic binary data validation
    if (data.size() < 8) {
      LOG_WARN("bitmap data too small to be valid");
      return false;
    }
    // Check for common image headers
    bool validHeader = false;
    if (data.substr(0, 8) == "\x89PNG\r\n\x1a\n")
      validHeader = true; // PNG
    if (data.substr(0, 2) == "\xFF\xD8")
      validHeader = true; // JPEG
    if (data.substr(0, 2) == "BM")
      validHeader = true; // BMP
    if (data.substr(0, 4) == "GIF8")
      validHeader = true; // GIF

    if (!validHeader) {
      LOG_WARN("bitmap data has unrecognized format");
      // Don't fail validation, just warn
    }
    break;
  }

  default:
    break;
  }

  return true;
}

std::string EiClipboard::sanitizeClipboardData(EFormat format, const std::string &data) const
{
  if (data.empty()) {
    return data;
  }

  std::string sanitized = data;

  switch (format) {
  case kText: {
    // Remove null bytes and control characters (except common ones)
    sanitized.erase(
        std::remove_if(
            sanitized.begin(), sanitized.end(),
            [](char c) { return (c == '\0' || (c > 0 && c < 32 && c != '\t' && c != '\n' && c != '\r')); }
        ),
        sanitized.end()
    );
    break;
  }

  case kHTML: {
    // Remove dangerous HTML elements and attributes
    std::regex scriptRegex(R"(<script[^>]*>.*?</script>)", std::regex_constants::icase);
    sanitized = std::regex_replace(sanitized, scriptRegex, "");

    std::regex jsRegex(R"(javascript:[^"'>\s]*)", std::regex_constants::icase);
    sanitized = std::regex_replace(sanitized, jsRegex, "");

    std::regex onEventRegex(R"(\ son\w+\s*=\s*[^>\s]*)", std::regex_constants::icase);
    sanitized = std::regex_replace(sanitized, onEventRegex, "");
    break;
  }

  case kBitmap: {
    // For binary data, just ensure it's not too large
    // Additional sanitization could include format-specific checks
    break;
  }

  default:
    break;
  }

  return sanitized;
}

bool EiClipboard::containsSensitiveData(const std::string &data) const
{
  if (data.empty()) {
    return false;
  }

  // Check for common patterns that might indicate sensitive data
  std::vector<std::regex> sensitivePatterns = {
      // Credit card numbers (basic pattern)
      std::regex(R"(\b\d{4}[\s-]?\d{4}[\s-]?\d{4}[\s-]?\d{4}\b)"),

      // Social Security Numbers (US format)
      std::regex(R"(\b\d{3}-\d{2}-\d{4}\b)"),

      // Email addresses (might be considered sensitive in some contexts)
      std::regex(R"(\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Z|a-z]{2,}\b)"),

      // Phone numbers (basic pattern)
      std::regex(R"(\b\d{3}[-.]?\d{3}[-.]?\d{4}\b)"),

      // Passwords (common patterns)
      std::regex(R"(password\s*[:=]\s*\S+)", std::regex_constants::icase),
      std::regex(R"(pwd\s*[:=]\s*\S+)", std::regex_constants::icase),

      // API keys (basic pattern)
      std::regex(R"(\b[A-Za-z0-9]{32,}\b)"), // 32+ character alphanumeric strings

      // URLs with authentication
      std::regex(R"(https?://[^:]+:[^@]+@)", std::regex_constants::icase)
  };

  for (const auto &pattern : sensitivePatterns) {
    if (std::regex_search(data, pattern)) {
      return true;
    }
  }

  return false;
}

std::string EiClipboard::calculateDataHash(const std::string &data) const
{
  // Simple hash calculation for change detection
  // In a production environment, you might want to use a proper hash function
  std::hash<std::string> hasher;
  size_t hashValue = hasher(data);

  std::stringstream ss;
  ss << std::hex << hashValue;
  return ss.str();
}

std::string EiClipboard::selectBestMimeType(EFormat format, const std::vector<std::string> &availableTypes) const
{
  if (!m_negotiator) {
    return formatToMimeType(format);
  }
  return m_negotiator->selectBestMimeType(format, availableTypes);
}

std::shared_ptr<EiClipboardSync> EiClipboard::getSync() const
{
  return m_sync;
}

void EiClipboard::setSyncOptimizationEnabled(bool enabled)
{
  m_syncOptimizationEnabled = enabled;
  LOG_DEBUG("clipboard sync optimization %s", enabled ? "enabled" : "disabled");
}

} // namespace deskflow
