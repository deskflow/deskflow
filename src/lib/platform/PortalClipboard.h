/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/Wayland.h"

#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#ifndef __APPLE__
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusPendingReply>
#include <QObject>
#endif

namespace deskflow {

#ifndef __APPLE__
//! XDG Desktop Portal clipboard interface wrapper using QDbus
/*!
This class provides a high-level interface to the XDG Desktop Portal
clipboard functionality using QDbus for D-Bus communication. It provides
async/sync conversion for clipboard operations and supports Gnome and KDE
desktop environments.
*/
class PortalClipboard : public QObject
{
  Q_OBJECT

public:
  //! Clipboard operation result
  struct ClipboardResult
  {
    bool success;
    std::string error;
    std::string data;
    std::string mimeType;
  };

  //! Callback for clipboard change notifications
  using ChangeCallback = std::function<void(const std::vector<std::string> &mimeTypes)>;

  PortalClipboard();
  ~PortalClipboard();

  //! Initialize portal clipboard interface
  bool initialize();

  //! Cleanup portal resources
  void cleanup();

  //! Check if portal clipboard interface is available
  bool isAvailable() const;

  //! Set clipboard data (async)
  std::future<ClipboardResult>
  setClipboardAsync(const std::string &mimeType, const std::string &data, const std::string &parentWindow = "");

  //! Get clipboard data (async)
  std::future<ClipboardResult> getClipboardAsync(const std::string &mimeType, const std::string &parentWindow = "");

  //! Set clipboard data (sync)
  ClipboardResult setClipboard(
      const std::string &mimeType, const std::string &data, const std::string &parentWindow = "", int timeoutMs = 5000
  );

  //! Get clipboard data (sync)
  ClipboardResult getClipboard(const std::string &mimeType, const std::string &parentWindow = "", int timeoutMs = 5000);

  //! Get available MIME types
  std::vector<std::string> getAvailableMimeTypes() const;

  //! Clear clipboard
  bool clearClipboard(const std::string &parentWindow = "");

  //! Set clipboard change callback
  void setChangeCallback(ChangeCallback callback);

  //! Start monitoring clipboard changes
  bool startMonitoring();

  //! Stop monitoring clipboard changes
  void stopMonitoring();

private Q_SLOTS:
  void onDbusCallFinished();

private:
  //! Check if portal service is available
  bool checkPortalService();

  //! Handle clipboard change notification
  void handleClipboardChange(const std::vector<std::string> &mimeTypes);

  //! Create parent window identifier for portal calls
  std::string createParentWindow(const std::string &parentWindow) const;

  //! Wait for async operation to complete
  ClipboardResult waitForOperation(std::shared_ptr<std::promise<ClipboardResult>> promise, int timeoutMs);

  //! Convert QVariant to clipboard result
  ClipboardResult processDbusReply(const QDBusPendingReply<QVariant> &reply, const std::string &operation);

  // D-Bus connection and interface
  QDBusConnection m_dbusConnection;
  std::unique_ptr<QDBusInterface> m_portalInterface;
  bool m_initialized;
  bool m_available;

  // Callback management
  ChangeCallback m_changeCallback;
  std::mutex m_callbackMutex;

  // Operation tracking
  std::mutex m_operationMutex;
  std::map<void *, std::shared_ptr<std::promise<ClipboardResult>>> m_pendingOperations;
};

#else
// Stub implementation for non-Linux builds
class PortalClipboard
{
public:
  struct ClipboardResult
  {
    bool success = false;
    std::string error = "Portal not available on this platform";
    std::string data;
    std::string mimeType;
  };

  using ChangeCallback = std::function<void(const std::vector<std::string> &)>;

  bool initialize()
  {
    return false;
  }
  void cleanup()
  {
  }
  bool isAvailable() const
  {
    return false;
  }

  ClipboardResult setClipboard(const std::string &, const std::string &, const std::string & = "", int = 5000)
  {
    return ClipboardResult{};
  }

  ClipboardResult getClipboard(const std::string &, const std::string & = "", int = 5000)
  {
    return ClipboardResult{};
  }

  std::vector<std::string> getAvailableMimeTypes() const
  {
    return {};
  }
  bool clearClipboard(const std::string & = "")
  {
    return false;
  }
  void setChangeCallback(ChangeCallback)
  {
  }
  bool startMonitoring()
  {
    return false;
  }
  void stopMonitoring()
  {
  }
};
#endif

} // namespace deskflow
