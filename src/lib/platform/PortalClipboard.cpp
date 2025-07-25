/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/PortalClipboard.h"
#include "base/Log.h"

#include <chrono>
#include <thread>

#ifndef DESKFLOW_UNIT_TESTING
#ifndef __APPLE__
#include <QDBusConnection>
#include <QDBusError>
#include <QDBusInterface>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QVariant>
#endif
#endif

namespace deskflow {

#ifndef __APPLE__

#ifndef DESKFLOW_UNIT_TESTING
// Full implementation for normal builds
PortalClipboard::PortalClipboard()
    : QObject(nullptr),
      m_dbusConnection(QDBusConnection::sessionBus()),
      m_initialized(false),
      m_available(false)
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
    if (!m_dbusConnection.isConnected()) {
      LOG_WARN("failed to connect to D-Bus session bus");
      return false;
    }

    // Check if portal service is available
    if (!checkPortalService()) {
      LOG_WARN("XDG Desktop Portal service not available");
      m_initialized = true;
      m_available = false;
      return false;
    }

    // Create interface for clipboard portal
    m_portalInterface = std::make_unique<QDBusInterface>(
        "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop", "org.freedesktop.portal.Clipboard",
        m_dbusConnection
    );

    m_available = m_portalInterface->isValid();
    m_initialized = true;

    if (m_available) {
      LOG_INFO("XDG-Clipboard portal interface initialized successfully");
    } else {
      LOG_WARN("XDG-Clipboard portal interface not available");
    }

    return m_available;
  } catch (...) {
    LOG_ERR("exception while initializing portal clipboard");
    m_initialized = true;
    m_available = false;
    return false;
  }
}

void PortalClipboard::cleanup()
{
  m_portalInterface.reset();
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

  // Implement XDG-Clipboard portal set operation using QDbus
  try {
    QVariantMap options;
    QString parent = QString::fromStdString(createParentWindow(parentWindow));

    // Store promise for async callback
    {
      std::lock_guard<std::mutex> lock(m_operationMutex);
      m_pendingOperations[promise.get()] = promise;
    }

    // Call the SetClipboard method
    QDBusPendingCall call = m_portalInterface->asyncCall(
        "SetClipboard", parent, options, QString::fromStdString(mimeType), QByteArray::fromStdString(data)
    );

    auto *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, promise, watcher]() {
      QDBusPendingReply<QVariant> reply = *watcher;
      ClipboardResult result = processDbusReply(reply, "SetClipboard");

      {
        std::lock_guard<std::mutex> lock(m_operationMutex);
        m_pendingOperations.erase(promise.get());
      }

      promise->set_value(result);
      watcher->deleteLater();
    });

  } catch (...) {
    LOG_ERR("exception in setClipboardAsync");
    promise->set_value(ClipboardResult{false, "D-Bus call failed", "", ""});
  }

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

  // Implement XDG-Clipboard portal get operation using QDbus
  try {
    QVariantMap options;
    QString parent = QString::fromStdString(createParentWindow(parentWindow));

    // Store promise for async callback
    {
      std::lock_guard<std::mutex> lock(m_operationMutex);
      m_pendingOperations[promise.get()] = promise;
    }

    // Call the GetClipboard method
    QDBusPendingCall call =
        m_portalInterface->asyncCall("GetClipboard", parent, options, QString::fromStdString(mimeType));

    auto *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, promise, watcher]() {
      QDBusPendingReply<QVariant> reply = *watcher;
      ClipboardResult result = processDbusReply(reply, "GetClipboard");

      {
        std::lock_guard<std::mutex> lock(m_operationMutex);
        m_pendingOperations.erase(promise.get());
      }

      promise->set_value(result);
      watcher->deleteLater();
    });

  } catch (...) {
    LOG_ERR("exception in getClipboardAsync");
    promise->set_value(ClipboardResult{false, "D-Bus call failed", "", ""});
  }

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

  // Implement XDG-Clipboard portal getAvailableMimeTypes operation using QDbus
  try {
    QVariantMap options;
    QDBusPendingReply<QStringList> reply = m_portalInterface->call("GetAvailableMimeTypes", options);
    reply.waitForFinished();

    if (reply.isError()) {
      QDBusError error = reply.error();
      LOG_WARN("D-Bus GetAvailableMimeTypes error: %s", error.message().toStdString().c_str());
      return {};
    } else {
      QStringList mimeTypes = reply.value();
      std::vector<std::string> result;
      for (const QString &mimeType : mimeTypes) {
        result.push_back(mimeType.toStdString());
      }
      return result;
    }
  } catch (...) {
    LOG_ERR("exception in getAvailableMimeTypes");
    return {};
  }
}

bool PortalClipboard::clearClipboard(const std::string &parentWindow)
{
  if (!isAvailable()) {
    return false;
  }

  // Implement XDG-Clipboard portal clear operation using QDbus
  try {
    QVariantMap options;
    QString parent = QString::fromStdString(createParentWindow(parentWindow));
    QDBusPendingReply<> reply = m_portalInterface->asyncCall("ClearClipboard", parent, options);

    auto *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [watcher]() {
      QDBusPendingReply<> reply = *watcher;
      if (reply.isError()) {
        QDBusError error = reply.error();
        LOG_WARN("D-Bus ClearClipboard error: %s", error.message().toStdString().c_str());
      } else {
        LOG_DEBUG("D-Bus ClearClipboard completed successfully");
      }
      watcher->deleteLater();
    });

    return true;
  } catch (...) {
    LOG_ERR("exception in clearClipboard");
    return false;
  }
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

// Removed onSetClipboardReady, onGetClipboardReady, and onClipboardChanged as they are not used by QDBus
// implementation.

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

bool PortalClipboard::checkPortalService()
{
  if (!m_dbusConnection.isConnected()) {
    return false;
  }

  // Check if the portal service is available
  QDBusInterface portalService(
      "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop", "org.freedesktop.DBus.Properties",
      m_dbusConnection
  );

  return portalService.isValid();
}

PortalClipboard::ClipboardResult
PortalClipboard::processDbusReply(const QDBusPendingReply<QVariant> &reply, const std::string &operation)
{
  ClipboardResult result;

  if (reply.isError()) {
    QDBusError error = reply.error();
    result.success = false;
    result.error = operation + " failed: " + error.message().toStdString();
    LOG_WARN("D-Bus %s error: %s", operation.c_str(), result.error.c_str());
  } else {
    result.success = true;
    // Process the reply data if needed
    QVariant replyData = reply.value();
    // For clipboard operations, we might get the data back
    if (replyData.canConvert<QByteArray>()) {
      QByteArray data = replyData.toByteArray();
      result.data = std::string(data.constData(), data.size());
    }
    LOG_DEBUG("D-Bus %s completed successfully", operation.c_str());
  }

  return result;
}

void PortalClipboard::onDbusCallFinished()
{
  // This slot can be used for additional D-Bus call handling if needed
}

#endif // !DESKFLOW_UNIT_TESTING

#endif // !__APPLE__

} // namespace deskflow
