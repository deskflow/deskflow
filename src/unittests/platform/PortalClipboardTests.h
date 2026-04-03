/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/Arch.h"
#include "base/Log.h"
#include "deskflow/ClipboardTypes.h"

#if WINAPI_LIBPORTAL
#include "platform/PortalClipboard.h"
#endif

#include <QTest>

//! Unit tests for PortalClipboard class
/*!
Tests the XDG Desktop Portal clipboard implementation. These tests verify:
- Correct D-Bus interface names and object paths are used
- MIME type handling works correctly for text/plain and other common types
- Graceful degradation when the portal is not available on the session bus
- Clipboard read/write operations follow the expected protocol

Note: Most tests require a running D-Bus session with xdg-desktop-portal.
Tests will be skipped if the portal clipboard is not available.
*/
class PortalClipboardTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  // Test constructor and basic availability (always run)
  void defaultCtor();
  void isAvailable();
  void isEnabled();

#if WINAPI_LIBPORTAL
  // Core clipboard functionality tests (require portal support)
  void initTestCase();
  void cleanupTestCase();
  void open();
  void close();
  void empty();
  void addText();
  void addHtml();
  void hasFormat();
  void getFormat();
  void getTime();
  void changed();
  void resetChanged();
  void monitoring();

  // Error handling tests
  void closeWithoutOpen();
  void addWithoutOpen();
  void getWithoutOpen();
  void emptyWithoutOpen();

  // MIME type conversion tests
  void mimeTypeConversion();
  void textMimeTypes();
  void htmlMimeType();
  void bitmapMimeType();
  void unknownMimeType();

private:
  Arch m_arch;
  Log m_log;

  const std::string m_testString = "deskflow portal test string";
  const std::string m_testString2 = "Another portal test string";
  const std::string m_testHtml = "<html><body>Test HTML</body></html>";

  // Helper to check if portal clipboard is actually available
  bool m_portalAvailable = false;
#endif
};
