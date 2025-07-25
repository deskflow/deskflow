/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "platform/Wayland.h"

#if WINAPI_LIBPORTAL
#include <functional>
#include <glib.h>
#include <libportal/portal.h>
#include <map>
#include <memory>
#include <string>
#include <vector>
#endif

namespace deskflow::test {

#if WINAPI_LIBPORTAL
//! Mock portal for testing clipboard functionality
/*!
This class provides a mock implementation of the XDG Desktop Portal
for testing purposes. It simulates portal behavior without requiring
an actual portal service to be running.
*/
class MockPortal
{
public:
  //! Callback type for clipboard change notifications
  using ClipboardChangeCallback = std::function<void(const std::vector<std::string> &)>;

  MockPortal();
  ~MockPortal();

  //! Initialize the mock portal
  bool initialize();

  //! Cleanup the mock portal
  void cleanup();

  //! Set clipboard data for a specific MIME type
  void setClipboardData(const std::string &mimeType, const std::string &data);

  //! Get clipboard data for a specific MIME type
  std::string getClipboardData(const std::string &mimeType) const;

  //! Get available MIME types
  std::vector<std::string> getAvailableMimeTypes() const;

  //! Clear clipboard data
  void clearClipboard();

  //! Set clipboard change callback
  void setClipboardChangeCallback(ClipboardChangeCallback callback);

  //! Simulate clipboard change from external application
  void simulateClipboardChange(const std::map<std::string, std::string> &data);

  //! Check if mock portal is available
  bool isAvailable() const;

  //! Get mock portal instance (for testing)
  XdpPortal *getPortalInstance() const;

private:
  //! Trigger clipboard change notification
  void notifyClipboardChange();

  //! Mock portal instance
  XdpPortal *m_mockPortal;

  //! Clipboard data storage
  std::map<std::string, std::string> m_clipboardData;

  //! Change callback
  ClipboardChangeCallback m_changeCallback;

  //! Initialization state
  bool m_initialized;
};

//! RAII helper for mock portal testing
class MockPortalScope
{
public:
  MockPortalScope();
  ~MockPortalScope();

  //! Get the mock portal instance
  MockPortal &portal();

  //! Check if mock portal is available
  bool isAvailable() const;

private:
  std::unique_ptr<MockPortal> m_mockPortal;
};

#else
// Stub implementations for non-portal builds
class MockPortal
{
public:
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
};

class MockPortalScope
{
public:
  MockPortal &portal()
  {
    static MockPortal stub;
    return stub;
  }
  bool isAvailable() const
  {
    return false;
  }
};
#endif

} // namespace deskflow::test
