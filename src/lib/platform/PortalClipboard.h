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

#ifdef DESKFLOW_UNIT_TESTING
// Stub implementation for unit testing
#include <QObject>  // Added for QObject inheritance
#include <QVariant> // Still needed for QVariant parameter

namespace deskflow {

class PortalClipboard : public QObject // Changed to inherit QObject
{
Q_OBJECT // Added Q_OBJECT macro

    public : struct ClipboardResult
  {
    bool success = true;
    std::string error = "";
    std::string data = "mocked data";
    std::string mimeType = "text/plain";
  };

  using ChangeCallback = std::function<void(const std::vector<std::string> &)>;

  PortalClipboard() : QObject(nullptr) // Modified to call QObject(nullptr)
  {
  }
  ~PortalClipboard() override // Added override
  {
  }

  bool initialize()
  {
    return true;
  }
  void cleanup()
  {
  }
  bool isAvailable() const
  {
    return true;
  }

  std::future<ClipboardResult>
  setClipboardAsync(const std::string &mimeType, const std::string &data, const std::string &parentWindow = "")
  {
    Q_UNUSED(parentWindow);
    auto promise = std::make_shared<std::promise<ClipboardResult>>();
    promise->set_value(ClipboardResult{true, "", data, mimeType});
    return promise->get_future();
  }

  std::future<ClipboardResult> getClipboardAsync(const std::string &mimeType, const std::string &parentWindow = "")
  {
    Q_UNUSED(parentWindow);
    auto promise = std::make_shared<std::promise<ClipboardResult>>();
    promise->set_value(ClipboardResult{true, "", "mocked data", mimeType});
    return promise->get_future();
  }

  ClipboardResult setClipboard(
      const std::string &mimeType, const std::string &data, const std::string &parentWindow = "", int timeoutMs = 5000
  )
  {
    Q_UNUSED(parentWindow);
    Q_UNUSED(timeoutMs);
    return ClipboardResult{true, "", data, mimeType};
  }

  ClipboardResult getClipboard(const std::string &mimeType, const std::string &parentWindow = "", int timeoutMs = 5000)
  {
    Q_UNUSED(parentWindow);
    Q_UNUSED(timeoutMs);
    return ClipboardResult{true, "", "mocked data", mimeType};
  }

  std::vector<std::string> getAvailableMimeTypes() const
  {
    return {"text/plain", "text/html"};
  }
  bool clearClipboard(const std::string &parentWindow = "")
  {
    Q_UNUSED(parentWindow);
    return true;
  }
  void setChangeCallback(ChangeCallback)
  {
  }
  bool startMonitoring()
  {
    return true;
  }
  void stopMonitoring()
  {
  }

private:
  bool checkPortalService()
  {
    return true;
  }
  std::string createParentWindow(const std::string &parentWindow) const
  {
    return parentWindow;
  }
  ClipboardResult processDbusReply(const QVariant &reply, const std::string &operation)
  {
    Q_UNUSED(reply);
    Q_UNUSED(operation);
    return ClipboardResult{};
  }
  void handleClipboardChange(const std::vector<std::string> &mimeTypes)
  {
    Q_UNUSED(mimeTypes);
  }
private Q_SLOTS:
  // Re-added for Q_OBJECT macro, even if empty in the stub
  void onDbusCallFinished()
  {
  }
};

} // namespace deskflow

#else // !DESKFLOW_UNIT_TESTING

// Original implementation for normal builds
#ifndef __APPLE__
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusPendingReply>
#include <QObject>
#endif

namespace deskflow {

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

  // D-Bus connection and interface
  QDBusConnection m_dbusConnection;
  std::unique_ptr<QDBusInterface> m_portalInterface;
  std::map<void *, std::shared_ptr<std::promise<ClipboardResult>>> m_pendingOperations;
  mutable std::mutex m_operationMutex;

  // Portal state
  bool m_initialized;
  bool m_available;

  // Callback for clipboard change notifications
  ChangeCallback m_changeCallback;
  mutable std::mutex m_callbackMutex;

  std::string createParentWindow(const std::string &parentWindow) const;
  ClipboardResult processDbusReply(const QDBusPendingReply<QVariant> &reply, const std::string &operation);
  void handleClipboardChange(const std::vector<std::string> &mimeTypes);
};

} // namespace deskflow

#endif // !DESKFLOW_UNIT_TESTING

#else
// Stub implementation for non-Linux builds (when __APPLE__ is defined)
namespace deskflow {

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

} // namespace deskflow
#endif
