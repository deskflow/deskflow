/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/PortalClipboard.h"
#include "base/Log.h"

#include <chrono>
#include <thread>

namespace deskflow {

#if WINAPI_LIBPORTAL

PortalClipboard::PortalClipboard() : m_portal(nullptr), m_initialized(false), m_available(false), m_changeSignalId(0)
{
}

PortalClipboard::~PortalClipboard()
{
  cleanup();
}

bool PortalClipboard::initialize()
{
  if (m_initialized) {
    return m_available;
  }

  try {
    m_portal = xdp_portal_new();
    if (!m_portal) {
      LOG_WARN("failed to create XDG portal for clipboard");
      return false;
    }

    // TODO: Check if clipboard interface is available when API exists
    // For now, we'll assume it's not available since the interface doesn't exist yet
    /*
    // Future implementation when clipboard portal interface is available:
    g_autoptr(GDBusProxy) proxy = g_dbus_proxy_new_for_bus_sync(
        G_BUS_TYPE_SESSION,
        G_DBUS_PROXY_FLAGS_NONE,
        nullptr,
        "org.freedesktop.portal.Desktop",
        "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.Clipboard",
        nullptr, nullptr);

    m_available = (proxy != nullptr);
    */

    m_available = false; // Set to true when interface becomes available
    m_initialized = true;

    if (m_available) {
      // TODO: Connect to clipboard change signal when available
      /*
      m_changeSignalId = g_signal_connect(m_portal, "clipboard-changed",
                                         G_CALLBACK(onClipboardChanged), this);
      */
      LOG_INFO("portal clipboard interface initialized");
    } else {
      LOG_INFO("portal clipboard interface not yet available");
    }

    return true;
  } catch (...) {
    LOG_ERR("exception while initializing portal clipboard");
    return false;
  }
}

void PortalClipboard::cleanup()
{
  if (m_portal) {
    if (m_changeSignalId > 0) {
      g_signal_handler_disconnect(m_portal, m_changeSignalId);
      m_changeSignalId = 0;
    }
    g_object_unref(m_portal);
    m_portal = nullptr;
  }

  m_initialized = false;
  m_available = false;

  // Cancel any pending operations
  std::lock_guard<std::mutex> lock(m_operationMutex);
  for (auto &pair : m_pendingOperations) {
    pair.second->set_value(ClipboardResult{false, "Portal cleanup", "", ""});
  }
  m_pendingOperations.clear();
}

bool PortalClipboard::isAvailable() const
{
  return m_initialized && m_available;
}

std::future<PortalClipboard::ClipboardResult> PortalClipboard::setClipboardAsync(
    const std::string &mimeType, const std::string &data, const std::string &parentWindow
)
{
  auto promise = std::make_shared<std::promise<ClipboardResult>>();
  auto future = promise->get_future();

  if (!isAvailable()) {
    promise->set_value(ClipboardResult{false, "Portal clipboard not available", "", ""});
    return future;
  }

  // TODO: Implement actual portal clipboard set when API is available
  /*
  // Future implementation:
  g_autoptr(GVariantBuilder) options = g_variant_builder_new(G_VARIANT_TYPE_VARDICT);

  GBytes* dataBytes = g_bytes_new(data.c_str(), data.size());

  std::string parent = createParentWindow(parentWindow);

  // Store promise for callback
  {
      std::lock_guard<std::mutex> lock(m_operationMutex);
      m_pendingOperations[promise.get()] = promise;
  }

  xdp_portal_set_clipboard_async(
      m_portal,
      parent.empty() ? nullptr : parent.c_str(),
      g_variant_builder_end(options),
      mimeType.c_str(),
      dataBytes,
      nullptr,
      onSetClipboardReady,
      promise.get());

  g_bytes_unref(dataBytes);
  */

  // Placeholder implementation
  LOG_DEBUG("setClipboardAsync called (not yet implemented)");
  promise->set_value(ClipboardResult{false, "Portal clipboard API not yet available", "", ""});

  return future;
}

std::future<PortalClipboard::ClipboardResult>
PortalClipboard::getClipboardAsync(const std::string &mimeType, const std::string &parentWindow)
{
  auto promise = std::make_shared<std::promise<ClipboardResult>>();
  auto future = promise->get_future();

  if (!isAvailable()) {
    promise->set_value(ClipboardResult{false, "Portal clipboard not available", "", ""});
    return future;
  }

  // TODO: Implement actual portal clipboard get when API is available
  /*
  // Future implementation:
  g_autoptr(GVariantBuilder) options = g_variant_builder_new(G_VARIANT_TYPE_VARDICT);

  const char* mimeTypes[] = { mimeType.c_str(), nullptr };

  std::string parent = createParentWindow(parentWindow);

  // Store promise for callback
  {
      std::lock_guard<std::mutex> lock(m_operationMutex);
      m_pendingOperations[promise.get()] = promise;
  }

  xdp_portal_get_clipboard_async(
      m_portal,
      parent.empty() ? nullptr : parent.c_str(),
      g_variant_builder_end(options),
      mimeTypes,
      nullptr,
      onGetClipboardReady,
      promise.get());
  */

  // Placeholder implementation
  LOG_DEBUG("getClipboardAsync called (not yet implemented)");
  promise->set_value(ClipboardResult{false, "Portal clipboard API not yet available", "", ""});

  return future;
}

PortalClipboard::ClipboardResult PortalClipboard::setClipboard(
    const std::string &mimeType, const std::string &data, const std::string &parentWindow, int timeoutMs
)
{
  auto future = setClipboardAsync(mimeType, data, parentWindow);

  if (future.wait_for(std::chrono::milliseconds(timeoutMs)) == std::future_status::timeout) {
    return ClipboardResult{false, "Operation timed out", "", ""};
  }

  return future.get();
}

PortalClipboard::ClipboardResult
PortalClipboard::getClipboard(const std::string &mimeType, const std::string &parentWindow, int timeoutMs)
{
  auto future = getClipboardAsync(mimeType, parentWindow);

  if (future.wait_for(std::chrono::milliseconds(timeoutMs)) == std::future_status::timeout) {
    return ClipboardResult{false, "Operation timed out", "", ""};
  }

  return future.get();
}

std::vector<std::string> PortalClipboard::getAvailableMimeTypes() const
{
  if (!isAvailable()) {
    return {};
  }

  // TODO: Implement when portal API is available
  /*
  // Future implementation:
  // Query portal for available MIME types
  */

  LOG_DEBUG("getAvailableMimeTypes called (not yet implemented)");
  return {};
}

bool PortalClipboard::clearClipboard(const std::string &parentWindow)
{
  if (!isAvailable()) {
    return false;
  }

  // TODO: Implement when portal API is available
  /*
  // Future implementation:
  // Call portal clear clipboard method
  */

  LOG_DEBUG("clearClipboard called (not yet implemented)");
  return false;
}

void PortalClipboard::setChangeCallback(ChangeCallback callback)
{
  std::lock_guard<std::mutex> lock(m_callbackMutex);
  m_changeCallback = std::move(callback);
}

bool PortalClipboard::startMonitoring()
{
  if (!isAvailable()) {
    return false;
  }

  // TODO: Enable clipboard change monitoring when API is available
  LOG_DEBUG("startMonitoring called (not yet implemented)");
  return false;
}

void PortalClipboard::stopMonitoring()
{
  if (!isAvailable()) {
    return;
  }

  // TODO: Disable clipboard change monitoring when API is available
  LOG_DEBUG("stopMonitoring called (not yet implemented)");
}

void PortalClipboard::onSetClipboardReady(GObject *source, GAsyncResult *result, gpointer user_data)
{
  // TODO: Implement when portal API is available
  /*
  auto* promise = static_cast<std::promise<ClipboardResult>*>(user_data);

  g_autoptr(GError) error = nullptr;
  gboolean success = xdp_portal_set_clipboard_finish(XDP_PORTAL(source), result, &error);

  ClipboardResult clipboardResult;
  clipboardResult.success = success;
  if (error) {
      clipboardResult.error = error->message;
  }

  promise->set_value(clipboardResult);
  */
}

void PortalClipboard::onGetClipboardReady(GObject *source, GAsyncResult *result, gpointer user_data)
{
  // TODO: Implement when portal API is available
  /*
  auto* promise = static_cast<std::promise<ClipboardResult>*>(user_data);

  g_autoptr(GError) error = nullptr;
  g_autofree char* mimeType = nullptr;
  GBytes* data = xdp_portal_get_clipboard_finish(XDP_PORTAL(source), result, &mimeType, &error);

  ClipboardResult clipboardResult;
  clipboardResult.success = (data != nullptr);
  if (error) {
      clipboardResult.error = error->message;
  } else if (data) {
      gsize size;
      const char* dataPtr = static_cast<const char*>(g_bytes_get_data(data, &size));
      clipboardResult.data = std::string(dataPtr, size);
      clipboardResult.mimeType = mimeType ? mimeType : "";
      g_bytes_unref(data);
  }

  promise->set_value(clipboardResult);
  */
}

void PortalClipboard::onClipboardChanged(XdpPortal *portal, const char *const *mime_types, gpointer user_data)
{
  auto *clipboard = static_cast<PortalClipboard *>(user_data);

  std::vector<std::string> mimeTypes;
  if (mime_types) {
    for (int i = 0; mime_types[i] != nullptr; ++i) {
      mimeTypes.emplace_back(mime_types[i]);
    }
  }

  clipboard->handleClipboardChange(mimeTypes);
}

void PortalClipboard::handleClipboardChange(const std::vector<std::string> &mimeTypes)
{
  LOG_DEBUG("clipboard changed, %zu MIME types available", mimeTypes.size());

  std::lock_guard<std::mutex> lock(m_callbackMutex);
  if (m_changeCallback) {
    try {
      m_changeCallback(mimeTypes);
    } catch (...) {
      LOG_ERR("exception in clipboard change callback");
    }
  }
}

std::string PortalClipboard::createParentWindow(const std::string &parentWindow) const
{
  if (parentWindow.empty()) {
    return "";
  }

  // TODO: Implement proper parent window handling
  // This should create a proper parent window identifier for portal calls
  return parentWindow;
}

#endif

} // namespace deskflow
