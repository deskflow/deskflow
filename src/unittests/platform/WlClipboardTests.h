/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "arch/Arch.h"
#include "base/Log.h"
#include "deskflow/ClipboardTypes.h"

#if WINAPI_LIBEI || WINAPI_PORTAL
#include "platform/WlClipboard.h"
#endif

#include <QTest>
#include <functional>

//! Unit tests for WlClipboard class
/*!
Tests the Wayland clipboard implementation that uses wl-copy and wl-paste
utilities for clipboard operations. These tests verify basic functionality
like opening/closing the clipboard, adding/getting data in different formats,
and monitoring clipboard changes.

Note: These tests require wl-clipboard tools to be installed and will be
skipped if they are not available or if Wayland support is not compiled in.
*/
class WlClipboardTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  // Test constructor and basic availability
  void defaultCtor();
  void isAvailable();

  // Core clipboard functionality tests
#if WINAPI_LIBEI || WINAPI_PORTAL
  void initTestCase();
  void cleanupTestCase();
  void open();
  void empty();
  void singleFormat();
  void multipleFormats();
  void hasFormat();
  void getTime();
  void monitoring();
  void primaryClipboard();
  void closeWithoutOpen();
  void addWithoutOpen();
  void getWithoutOpen();
#endif

private:
  Arch m_arch;
  Log m_log;

#if WINAPI_LIBEI || WINAPI_PORTAL
  const std::string m_testString = "deskflow test string";
  const std::string m_testString2 = "Another test string";
  const std::string m_testHtml = "<html><body>Test HTML</body></html>";

  // clang-format off
  // Helper methods for robust testing
  bool waitForClipboardCondition(WlClipboard &clipboard, std::function<bool()> condition, int timeoutMs = 2000);
  bool waitForClipboardEmpty(WlClipboard &clipboard, const std::string &previousContent, int timeoutMs = 2000);
  bool waitForClipboardContent(WlClipboard &clipboard, IClipboard::Format format, const std::string &expectedContent, int timeoutMs = 2000);
  // clang-format on
#endif
};
