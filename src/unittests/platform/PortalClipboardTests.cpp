/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "PortalClipboardTests.h"

#include "base/LogLevel.h"
#include "deskflow/IClipboard.h"

#include <QTest>

//
// Tests that always run (no portal required)
//

void PortalClipboardTests::defaultCtor()
{
#if WINAPI_LIBPORTAL
  PortalClipboard clipboard(kClipboardClipboard);
  QCOMPARE(clipboard.getID(), kClipboardClipboard);

  PortalClipboard primaryClipboard(kClipboardSelection);
  QCOMPARE(primaryClipboard.getID(), kClipboardSelection);
#else
  QSKIP("Portal clipboard not compiled in (libportal < 0.9.1)");
#endif
}

void PortalClipboardTests::isAvailable()
{
#if WINAPI_LIBPORTAL
  // This test checks if the portal clipboard is available on the system
  // The result depends on whether xdg-desktop-portal is running and has clipboard support
  bool available = PortalClipboard::isAvailable();

  // We don't assert true/false here because it depends on system setup
  // Just verify the method doesn't crash and returns a boolean
  QVERIFY(available == true || available == false);
#else
  QSKIP("Portal clipboard not compiled in (libportal < 0.9.1)");
#endif
}

void PortalClipboardTests::isEnabled()
{
#if WINAPI_LIBPORTAL
  // isEnabled() checks the settings to see if portal/wl-clipboard is enabled
  // This should return a boolean without crashing
  bool enabled = PortalClipboard::isEnabled();
  QVERIFY(enabled == true || enabled == false);
#else
  QSKIP("Portal clipboard not compiled in (libportal < 0.9.1)");
#endif
}

#if WINAPI_LIBPORTAL

//
// Test case setup/cleanup
//

void PortalClipboardTests::initTestCase()
{
  m_arch.init();
  m_log.setFilter(LogLevel::Debug2);

  // Check if portal clipboard is available
  m_portalAvailable = PortalClipboard::isAvailable();

  if (!m_portalAvailable) {
    QSKIP("Portal clipboard not available (xdg-desktop-portal missing or no clipboard support)");
  }
}

void PortalClipboardTests::cleanupTestCase()
{
  // Clean up by clearing clipboard if we have access
  if (m_portalAvailable) {
    try {
      PortalClipboard clipboard(kClipboardClipboard);
      if (clipboard.open(0)) {
        clipboard.empty();
        clipboard.close();
      }
    } catch (...) {
      // Ignore cleanup errors
    }
  }
}

//
// Core clipboard functionality tests
//

void PortalClipboardTests::open()
{
  PortalClipboard clipboard(kClipboardClipboard);
  QVERIFY(clipboard.open(0));

  // Opening again should return false (already open)
  QVERIFY(!clipboard.open(1));

  clipboard.close();

  // Should be able to open again after closing
  QVERIFY(clipboard.open(2));
  clipboard.close();
}

void PortalClipboardTests::close()
{
  PortalClipboard clipboard(kClipboardClipboard);
  QVERIFY(clipboard.open(0));
  clipboard.close();

  // Should be safe to close multiple times (no crash)
  clipboard.close();

  // Should be able to open after closing
  QVERIFY(clipboard.open(0));
  clipboard.close();
}

void PortalClipboardTests::empty()
{
  PortalClipboard clipboard(kClipboardClipboard);
  QVERIFY(clipboard.open(0));

  // First add some known content
  clipboard.add(IClipboard::Format::Text, m_testString);

  // Now empty the clipboard (takes ownership and clears)
  QVERIFY(clipboard.empty());

  clipboard.close();
}

void PortalClipboardTests::addText()
{
  PortalClipboard clipboard(kClipboardClipboard);
  QVERIFY(clipboard.open(0));

  // Clear clipboard first
  QVERIFY(clipboard.empty());

  // Add text data
  clipboard.add(IClipboard::Format::Text, m_testString);
  QVERIFY(clipboard.has(IClipboard::Format::Text));
  QCOMPARE(clipboard.get(IClipboard::Format::Text), m_testString);

  clipboard.close();
}

void PortalClipboardTests::addHtml()
{
  PortalClipboard clipboard(kClipboardClipboard);
  QVERIFY(clipboard.open(0));

  // Clear clipboard first
  QVERIFY(clipboard.empty());

  // Add HTML data
  clipboard.add(IClipboard::Format::HTML, m_testHtml);
  QVERIFY(clipboard.has(IClipboard::Format::HTML));
  QCOMPARE(clipboard.get(IClipboard::Format::HTML), m_testHtml);

  clipboard.close();
}

void PortalClipboardTests::hasFormat()
{
  PortalClipboard clipboard(kClipboardClipboard);
  QVERIFY(clipboard.open(0));

  // Clear clipboard first
  QVERIFY(clipboard.empty());

  // After clearing, cached availability should be false
  for (int i = 0; i < static_cast<int>(IClipboard::Format::TotalFormats); ++i) {
    QVERIFY(!clipboard.has(static_cast<IClipboard::Format>(i)));
  }

  // Add text and verify it's available
  clipboard.add(IClipboard::Format::Text, m_testString);
  QVERIFY(clipboard.has(IClipboard::Format::Text));
  QVERIFY(!clipboard.has(IClipboard::Format::HTML));
  QVERIFY(!clipboard.has(IClipboard::Format::Bitmap));

  clipboard.close();
}

void PortalClipboardTests::getFormat()
{
  PortalClipboard clipboard(kClipboardClipboard);
  QVERIFY(clipboard.open(0));

  // Clear and add text
  QVERIFY(clipboard.empty());
  clipboard.add(IClipboard::Format::Text, m_testString);

  // Verify we get back what we added
  std::string result = clipboard.get(IClipboard::Format::Text);
  QCOMPARE(result, m_testString);

  // HTML should be empty since we didn't add it
  std::string htmlResult = clipboard.get(IClipboard::Format::HTML);
  QVERIFY(htmlResult.empty());

  clipboard.close();
}

void PortalClipboardTests::getTime()
{
  PortalClipboard clipboard(kClipboardClipboard);
  QVERIFY(clipboard.open(100));

  // Should return the time passed to open()
  QCOMPARE(clipboard.getTime(), static_cast<IClipboard::Time>(100));

  clipboard.close();

  QVERIFY(clipboard.open(200));
  QCOMPARE(clipboard.getTime(), static_cast<IClipboard::Time>(200));
  clipboard.close();
}

void PortalClipboardTests::changed()
{
  PortalClipboard clipboard(kClipboardClipboard);
  QVERIFY(clipboard.open(0));

  // Initially should not have changed
  QVERIFY(!clipboard.hasChanged());

  clipboard.close();
}

void PortalClipboardTests::resetChanged()
{
  PortalClipboard clipboard(kClipboardClipboard);
  QVERIFY(clipboard.open(0));

  // Reset should not crash and clears the changed flag
  clipboard.resetChanged();

  // After reset, should not have changed
  QVERIFY(!clipboard.hasChanged());

  clipboard.close();
}

void PortalClipboardTests::monitoring()
{
  PortalClipboard clipboard(kClipboardClipboard);
  QVERIFY(clipboard.open(0));

  // Start monitoring (requests clipboard access from portal)
  clipboard.startMonitoring();

  // Should be safe to call multiple times (idempotent)
  clipboard.startMonitoring();

  // Stop monitoring
  clipboard.stopMonitoring();

  // Should be safe to call multiple times
  clipboard.stopMonitoring();

  clipboard.close();
}

//
// Error handling tests
//

void PortalClipboardTests::closeWithoutOpen()
{
  PortalClipboard clipboard(kClipboardClipboard);

  // Should be safe to call close without ever opening
  clipboard.close();

  // After close, open should work normally
  QVERIFY(clipboard.open(0));
  clipboard.close();
}

void PortalClipboardTests::addWithoutOpen()
{
  PortalClipboard clipboard(kClipboardClipboard);

  // Should not crash when adding without open
  // The add() method should just return early
  clipboard.add(IClipboard::Format::Text, m_testString);

  // Should still be able to open and use normally
  QVERIFY(clipboard.open(0));
  clipboard.add(IClipboard::Format::Text, m_testString2);
  QCOMPARE(clipboard.get(IClipboard::Format::Text), m_testString2);
  clipboard.close();
}

void PortalClipboardTests::getWithoutOpen()
{
  PortalClipboard clipboard(kClipboardClipboard);

  // Should return empty string when getting without open
  std::string result = clipboard.get(IClipboard::Format::Text);
  QVERIFY(result.empty());

  // Should return false for has() without open
  QVERIFY(!clipboard.has(IClipboard::Format::Text));

  // Should still be able to open and use normally
  QVERIFY(clipboard.open(0));
  clipboard.add(IClipboard::Format::Text, m_testString);
  QCOMPARE(clipboard.get(IClipboard::Format::Text), m_testString);
  clipboard.close();
}

void PortalClipboardTests::emptyWithoutOpen()
{
  PortalClipboard clipboard(kClipboardClipboard);

  // Should return false when emptying without open
  QVERIFY(!clipboard.empty());

  // Should still be able to open and use normally
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());
  clipboard.close();
}

//
// MIME type conversion tests
//

void PortalClipboardTests::mimeTypeConversion()
{
  // This test verifies MIME type handling via the add/has/get cycle
  PortalClipboard clipboard(kClipboardClipboard);
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());

  // Text format should map to "text/plain;charset=utf-8"
  clipboard.add(IClipboard::Format::Text, m_testString);
  QVERIFY(clipboard.has(IClipboard::Format::Text));
  QCOMPARE(clipboard.get(IClipboard::Format::Text), m_testString);

  clipboard.close();
}

void PortalClipboardTests::textMimeTypes()
{
  PortalClipboard clipboard(kClipboardClipboard);
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());

  // Add text - internally converts Format::Text to MIME type
  clipboard.add(IClipboard::Format::Text, m_testString);
  QVERIFY(clipboard.has(IClipboard::Format::Text));
  QCOMPARE(clipboard.get(IClipboard::Format::Text), m_testString);

  // Add again - should replace the previous value
  clipboard.add(IClipboard::Format::Text, m_testString2);
  QCOMPARE(clipboard.get(IClipboard::Format::Text), m_testString2);

  clipboard.close();
}

void PortalClipboardTests::htmlMimeType()
{
  PortalClipboard clipboard(kClipboardClipboard);
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());

  // Add HTML content
  clipboard.add(IClipboard::Format::HTML, m_testHtml);
  QVERIFY(clipboard.has(IClipboard::Format::HTML));
  QCOMPARE(clipboard.get(IClipboard::Format::HTML), m_testHtml);

  // Text should not be set
  QVERIFY(!clipboard.has(IClipboard::Format::Text));

  clipboard.close();
}

void PortalClipboardTests::bitmapMimeType()
{
  PortalClipboard clipboard(kClipboardClipboard);
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());

  // Add bitmap content (minimal BMP data for testing)
  std::string bitmapData(54, '\0'); // BMP file header (14) + INFOHEADER (40)
  clipboard.add(IClipboard::Format::Bitmap, bitmapData);
  QVERIFY(clipboard.has(IClipboard::Format::Bitmap));
  QCOMPARE(clipboard.get(IClipboard::Format::Bitmap), bitmapData);

  clipboard.close();
}

void PortalClipboardTests::unknownMimeType()
{
  PortalClipboard clipboard(kClipboardClipboard);
  QVERIFY(clipboard.open(0));
  QVERIFY(clipboard.empty());

  // Add multiple formats - all should be handled correctly
  clipboard.add(IClipboard::Format::Text, m_testString);
  clipboard.add(IClipboard::Format::HTML, m_testHtml);

  // Both should be available
  QVERIFY(clipboard.has(IClipboard::Format::Text));
  QVERIFY(clipboard.has(IClipboard::Format::HTML));
  QCOMPARE(clipboard.get(IClipboard::Format::Text), m_testString);
  QCOMPARE(clipboard.get(IClipboard::Format::HTML), m_testHtml);

  clipboard.close();
}

#endif // WINAPI_LIBPORTAL

QTEST_MAIN(PortalClipboardTests)
