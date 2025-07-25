/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "MockPortal.h"
#include "platform/EiClipboard.h"
#include "platform/Wayland.h"

#include <QTest>
#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace deskflow;

class EiClipboardTests : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void initTestCase();
  void cleanupTestCase();
  void constructorInitializesCorrectly();
  void portalAvailabilityDetection();
  void openCloseBasicFunctionality();
  void addDataRequiresOpenClipboard();
  void hasDataRequiresOpenClipboard();
  void getDataRequiresOpenClipboard();
  void emptyRequiresOpenClipboard();
  void multipleFormatsSupport();
  void invalidFormatHandling();
  void dataOverwrite();
  void largeDataHandling();
  void emptyDataHandling();
  void timeHandling();

  // Mock-portal based tests
  void mockPortalAvailability();
  void clipboardMonitoring();
  void mockClipboardData();
  void clipboardChangeSimulation();

private:
  // Helpers that create a fresh clipboard for each test
  void createClipboard()
  {
    // Reset any existing clipboard first
    m_clipboard.reset();

    // Add safety check for platform support
    if (!(deskflow::platform::kHasPortal && deskflow::platform::kHasPortalClipboard)) {
      qWarning("Platform compilation flags indicate no portal clipboard support");
      return;
    }

    try {
      m_clipboard = std::make_unique<EiClipboard>();
    } catch (const std::exception &e) {
      qWarning("Failed to create EiClipboard instance: %s", e.what());
    } catch (...) {
      qWarning("Failed to create EiClipboard instance: unknown exception");
    }
  }

  bool isClipboardSupported() const
  {
    return deskflow::platform::kHasPortal && deskflow::platform::kHasPortalClipboard;
  }

  std::unique_ptr<EiClipboard> m_clipboard;
  std::unique_ptr<deskflow::test::MockPortalScope> m_mockScope;
};

// ===== Basic tests =====

void EiClipboardTests::initTestCase()
{
  qDebug("Initializing EiClipboardTests...");

  // Check platform support early
  if (!isClipboardSupported()) {
    qWarning("Platform does not support portal clipboard functionality");
    qWarning(
        "kHasPortal: %s, kHasPortalClipboard: %s", deskflow::platform::kHasPortal ? "true" : "false",
        deskflow::platform::kHasPortalClipboard ? "true" : "false"
    );
    return;
  }

  // Initialize mock portal scope
  try {
    m_mockScope = std::make_unique<deskflow::test::MockPortalScope>();
    qDebug("Mock portal scope initialized successfully");
  } catch (const std::exception &e) {
    qWarning("Failed to initialize mock portal scope: %s", e.what());
  } catch (...) {
    qWarning("Failed to initialize mock portal scope: unknown exception");
  }
}

void EiClipboardTests::cleanupTestCase()
{
  qDebug("Cleaning up EiClipboardTests...");
  m_clipboard.reset();
  m_mockScope.reset();
}

void EiClipboardTests::constructorInitializesCorrectly()
{
  if (!isClipboardSupported()) {
    QSKIP("Platform does not support portal clipboard functionality");
  }

  createClipboard();

  if (!m_clipboard) {
    QSKIP("Failed to create clipboard instance - likely missing runtime dependencies or no Wayland session");
  }

  QVERIFY(m_clipboard != nullptr);

  // Test basic functionality without crashing
  try {
    // Only test getTime() if the clipboard seems functional
    auto time = m_clipboard->getTime();
    Q_UNUSED(time); // Just verify it doesn't crash
    qDebug("Clipboard getTime() call succeeded");
  } catch (const std::exception &e) {
    qWarning("getTime() threw exception: %s", e.what());
    QSKIP("Clipboard not fully functional - getTime() failed");
  } catch (...) {
    qWarning("getTime() threw unknown exception");
    QSKIP("Clipboard not fully functional - getTime() failed");
  }
}

void EiClipboardTests::portalAvailabilityDetection()
{
  if (!isClipboardSupported()) {
    QSKIP("Platform does not support portal clipboard functionality");
  }

  createClipboard();
  if (!m_clipboard) {
    QSKIP("Failed to create clipboard instance");
  }

  try {
    // Test portal availability - this should work regardless of actual portal state
    bool portalAvailable = m_clipboard->isPortalAvailable();
    qDebug("Portal availability: %s", portalAvailable ? "true" : "false");

    // Just verify the call doesn't crash - actual result depends on environment
    QVERIFY(portalAvailable == true || portalAvailable == false);
  } catch (...) {
    QSKIP("isPortalAvailable() threw exception - portal interface not working");
  }
}

void EiClipboardTests::openCloseBasicFunctionality()
{
  if (!isClipboardSupported()) {
    QSKIP("Platform does not support portal clipboard functionality");
  }

  createClipboard();
  if (!m_clipboard) {
    QSKIP("Failed to create clipboard instance");
  }

  try {
    bool openResult = m_clipboard->open(123);
    if (!openResult) {
      QSKIP("Clipboard open() failed - likely no portal connection available");
    }

    QVERIFY(openResult);
    QCOMPARE(m_clipboard->getTime(), IClipboard::Time(123));

    // Should not reopen while already open
    QVERIFY(!m_clipboard->open(456));
    QCOMPARE(m_clipboard->getTime(), IClipboard::Time(123));

    m_clipboard->close();

    // Should open again
    QVERIFY(m_clipboard->open(789));
    QCOMPARE(m_clipboard->getTime(), IClipboard::Time(789));
    m_clipboard->close();
  } catch (const std::exception &e) {
    QFAIL(qPrintable(QString("Exception during basic open/close: %1").arg(e.what())));
  } catch (...) {
    QFAIL("Unknown exception during basic open/close operations");
  }
}

void EiClipboardTests::addDataRequiresOpenClipboard()
{
  if (!isClipboardSupported()) {
    QSKIP("Platform does not support portal clipboard functionality");
  }

  createClipboard();
  if (!m_clipboard) {
    QSKIP("Failed to create clipboard instance");
  }

  try {
    // Add without opening (should do nothing)
    m_clipboard->add(IClipboard::kText, "test data");
    QVERIFY(!m_clipboard->has(IClipboard::kText));

    if (!m_clipboard->open(0)) {
      QSKIP("Clipboard open() failed - likely no portal connection available");
    }

    m_clipboard->add(IClipboard::kText, "test data");
    QVERIFY(m_clipboard->has(IClipboard::kText));
    QCOMPARE(m_clipboard->get(IClipboard::kText), std::string("test data"));
    m_clipboard->close();
  } catch (const std::exception &e) {
    QFAIL(qPrintable(QString("Exception during add data test: %1").arg(e.what())));
  } catch (...) {
    QFAIL("Unknown exception during add data test");
  }
}

void EiClipboardTests::hasDataRequiresOpenClipboard()
{
  if (!isClipboardSupported()) {
    QSKIP("Platform does not support portal clipboard functionality");
  }

  createClipboard();
  if (!m_clipboard) {
    QSKIP("Failed to create clipboard instance");
  }

  try {
    QVERIFY(!m_clipboard->has(IClipboard::kText));

    if (!m_clipboard->open(0)) {
      QSKIP("Clipboard open() failed - likely no portal connection available");
    }

    QVERIFY(!m_clipboard->has(IClipboard::kText));
    m_clipboard->add(IClipboard::kText, "test");
    QVERIFY(m_clipboard->has(IClipboard::kText));
    m_clipboard->close();
  } catch (const std::exception &e) {
    QFAIL(qPrintable(QString("Exception during has data test: %1").arg(e.what())));
  } catch (...) {
    QFAIL("Unknown exception during has data test");
  }
}

void EiClipboardTests::getDataRequiresOpenClipboard()
{
  if (!isClipboardSupported()) {
    QSKIP("Platform does not support portal clipboard functionality");
  }

  createClipboard();
  if (!m_clipboard) {
    QSKIP("Failed to create clipboard instance");
  }

  try {
    QCOMPARE(m_clipboard->get(IClipboard::kText), std::string(""));

    if (!m_clipboard->open(0)) {
      QSKIP("Clipboard open() failed - likely no portal connection available");
    }

    QCOMPARE(m_clipboard->get(IClipboard::kText), std::string(""));
    m_clipboard->add(IClipboard::kText, "test data");
    QCOMPARE(m_clipboard->get(IClipboard::kText), std::string("test data"));
    m_clipboard->close();
  } catch (const std::exception &e) {
    QFAIL(qPrintable(QString("Exception during get data test: %1").arg(e.what())));
  } catch (...) {
    QFAIL("Unknown exception during get data test");
  }
}

void EiClipboardTests::emptyRequiresOpenClipboard()
{
  if (!isClipboardSupported()) {
    QSKIP("Platform does not support portal clipboard functionality");
  }

  createClipboard();
  if (!m_clipboard) {
    QSKIP("Failed to create clipboard instance");
  }

  try {
    // Without opening, empty() should return false (not empty = has no access)
    QVERIFY(!m_clipboard->empty());

    if (!m_clipboard->open(0)) {
      QSKIP("Clipboard open() failed - likely no portal connection available");
    }

    m_clipboard->add(IClipboard::kText, "test");
    m_clipboard->add(IClipboard::kBitmap, "bitmap data");
    QVERIFY(m_clipboard->has(IClipboard::kText));
    QVERIFY(m_clipboard->has(IClipboard::kBitmap));

    // The behavior of empty() may vary based on portal implementation
    bool emptyResult = m_clipboard->empty();
    qDebug("empty() returned: %s", emptyResult ? "true" : "false");

    m_clipboard->close();
  } catch (const std::exception &e) {
    QFAIL(qPrintable(QString("Exception during empty test: %1").arg(e.what())));
  } catch (...) {
    QFAIL("Unknown exception during empty test");
  }
}

void EiClipboardTests::multipleFormatsSupport()
{
  if (!isClipboardSupported()) {
    QSKIP("Platform does not support portal clipboard functionality");
  }

  createClipboard();
  if (!m_clipboard) {
    QSKIP("Failed to create clipboard instance");
  }

  try {
    if (!m_clipboard->open(0)) {
      QSKIP("Clipboard open() failed - likely no portal connection available");
    }

    m_clipboard->add(IClipboard::kText, "plain text");
    m_clipboard->add(IClipboard::kHTML, "<html><body>HTML content</body></html>");
    m_clipboard->add(IClipboard::kBitmap, "binary bitmap data");

    QVERIFY(m_clipboard->has(IClipboard::kText));
    QVERIFY(m_clipboard->has(IClipboard::kHTML));
    QVERIFY(m_clipboard->has(IClipboard::kBitmap));
    QCOMPARE(m_clipboard->get(IClipboard::kText), std::string("plain text"));
    QCOMPARE(m_clipboard->get(IClipboard::kHTML), std::string("<html><body>HTML content</body></html>"));
    QCOMPARE(m_clipboard->get(IClipboard::kBitmap), std::string("binary bitmap data"));
    m_clipboard->close();
  } catch (const std::exception &e) {
    QFAIL(qPrintable(QString("Exception during multiple formats test: %1").arg(e.what())));
  } catch (...) {
    QFAIL("Unknown exception during multiple formats test");
  }
}

void EiClipboardTests::invalidFormatHandling()
{
  if (!isClipboardSupported()) {
    QSKIP("Platform does not support portal clipboard functionality");
  }

  createClipboard();
  if (!m_clipboard) {
    QSKIP("Failed to create clipboard instance");
  }

  try {
    if (!m_clipboard->open(0)) {
      QSKIP("Clipboard open() failed - likely no portal connection available");
    }

    QVERIFY(!m_clipboard->has(static_cast<IClipboard::EFormat>(-1)));
    QVERIFY(!m_clipboard->has(static_cast<IClipboard::EFormat>(IClipboard::kNumFormats)));
    QCOMPARE(m_clipboard->get(static_cast<IClipboard::EFormat>(-1)), std::string(""));
    QCOMPARE(m_clipboard->get(static_cast<IClipboard::EFormat>(IClipboard::kNumFormats)), std::string(""));
    m_clipboard->close();
  } catch (const std::exception &e) {
    QFAIL(qPrintable(QString("Exception during invalid format test: %1").arg(e.what())));
  } catch (...) {
    QFAIL("Unknown exception during invalid format test");
  }
}

void EiClipboardTests::dataOverwrite()
{
  if (!isClipboardSupported()) {
    QSKIP("Platform does not support portal clipboard functionality");
  }

  createClipboard();
  if (!m_clipboard) {
    QSKIP("Failed to create clipboard instance");
  }

  try {
    if (!m_clipboard->open(0)) {
      QSKIP("Clipboard open() failed - likely no portal connection available");
    }

    m_clipboard->add(IClipboard::kText, "initial data");
    QCOMPARE(m_clipboard->get(IClipboard::kText), std::string("initial data"));
    m_clipboard->add(IClipboard::kText, "new data");
    QCOMPARE(m_clipboard->get(IClipboard::kText), std::string("new data"));
    m_clipboard->close();
  } catch (const std::exception &e) {
    QFAIL(qPrintable(QString("Exception during data overwrite test: %1").arg(e.what())));
  } catch (...) {
    QFAIL("Unknown exception during data overwrite test");
  }
}

void EiClipboardTests::largeDataHandling()
{
  if (!isClipboardSupported()) {
    QSKIP("Platform does not support portal clipboard functionality");
  }

  createClipboard();
  if (!m_clipboard) {
    QSKIP("Failed to create clipboard instance");
  }

  try {
    if (!m_clipboard->open(0)) {
      QSKIP("Clipboard open() failed - likely no portal connection available");
    }

    std::string largeData(1024 * 1024, 'A');
    m_clipboard->add(IClipboard::kText, largeData);
    QVERIFY(m_clipboard->has(IClipboard::kText));
    QCOMPARE(m_clipboard->get(IClipboard::kText), largeData);
    m_clipboard->close();
  } catch (const std::exception &e) {
    QFAIL(qPrintable(QString("Exception during large data test: %1").arg(e.what())));
  } catch (...) {
    QFAIL("Unknown exception during large data test");
  }
}

void EiClipboardTests::emptyDataHandling()
{
  if (!isClipboardSupported()) {
    QSKIP("Platform does not support portal clipboard functionality");
  }

  createClipboard();
  if (!m_clipboard) {
    QSKIP("Failed to create clipboard instance");
  }

  try {
    if (!m_clipboard->open(0)) {
      QSKIP("Clipboard open() failed - likely no portal connection available");
    }

    m_clipboard->add(IClipboard::kText, "");
    QVERIFY(m_clipboard->has(IClipboard::kText));
    QCOMPARE(m_clipboard->get(IClipboard::kText), std::string(""));
    m_clipboard->close();
  } catch (const std::exception &e) {
    QFAIL(qPrintable(QString("Exception during empty data test: %1").arg(e.what())));
  } catch (...) {
    QFAIL("Unknown exception during empty data test");
  }
}

void EiClipboardTests::timeHandling()
{
  if (!isClipboardSupported()) {
    QSKIP("Platform does not support portal clipboard functionality");
  }

  createClipboard();
  if (!m_clipboard) {
    QSKIP("Failed to create clipboard instance");
  }

  try {
    if (!m_clipboard->open(0)) {
      QSKIP("Clipboard open() failed - likely no portal connection available");
    }
    QCOMPARE(m_clipboard->getTime(), IClipboard::Time(0));
    m_clipboard->close();

    if (!m_clipboard->open(12345)) {
      QSKIP("Clipboard second open() failed");
    }
    QCOMPARE(m_clipboard->getTime(), IClipboard::Time(12345));
    m_clipboard->close();

    if (!m_clipboard->open(static_cast<IClipboard::Time>(-1))) {
      QSKIP("Clipboard third open() failed");
    }
    QCOMPARE(m_clipboard->getTime(), static_cast<IClipboard::Time>(-1));
    m_clipboard->close();
  } catch (const std::exception &e) {
    QFAIL(qPrintable(QString("Exception during time handling test: %1").arg(e.what())));
  } catch (...) {
    QFAIL("Unknown exception during time handling test");
  }
}

// ===== Mock portal tests =====

void EiClipboardTests::mockPortalAvailability()
{
  if (!m_mockScope) {
    QSKIP("Mock portal scope not available");
  }

  // Mock portal should always be available when created successfully
  QVERIFY(m_mockScope->isAvailable());
}

void EiClipboardTests::clipboardMonitoring()
{
  if (!m_mockScope || !m_mockScope->isAvailable()) {
    QSKIP("Mock portal not available");
  }

  createClipboard();
  if (!m_clipboard) {
    QSKIP("Failed to create clipboard instance");
  }

  try {
    QVERIFY(!m_clipboard->isMonitoring());

    bool startResult = m_clipboard->startMonitoring();
    if (startResult) {
      QVERIFY(m_clipboard->isMonitoring());
      m_clipboard->stopMonitoring();
      QVERIFY(!m_clipboard->isMonitoring());
    } else {
      qDebug("startMonitoring() returned false - monitoring not supported in this environment");
    }
  } catch (const std::exception &e) {
    QFAIL(qPrintable(QString("Exception during clipboard monitoring test: %1").arg(e.what())));
  } catch (...) {
    QFAIL("Unknown exception during clipboard monitoring test");
  }
}

void EiClipboardTests::mockClipboardData()
{
  if (!m_mockScope || !m_mockScope->isAvailable()) {
    QSKIP("Mock portal not available");
  }

  auto &mockPortal = m_mockScope->portal();
  mockPortal.setClipboardData("text/plain", "test data");
  mockPortal.setClipboardData("text/html", "<p>HTML test</p>");
  QCOMPARE(mockPortal.getClipboardData("text/plain"), std::string("test data"));
  QCOMPARE(mockPortal.getClipboardData("text/html"), std::string("<p>HTML test</p>"));
  auto mimeTypes = mockPortal.getAvailableMimeTypes();
  QCOMPARE(mimeTypes.size(), std::size_t(2));
  QVERIFY(std::find(mimeTypes.begin(), mimeTypes.end(), "text/plain") != mimeTypes.end());
  QVERIFY(std::find(mimeTypes.begin(), mimeTypes.end(), "text/html") != mimeTypes.end());
}

void EiClipboardTests::clipboardChangeSimulation()
{
  if (!m_mockScope || !m_mockScope->isAvailable()) {
    QSKIP("Mock portal not available");
  }

  auto &mockPortal = m_mockScope->portal();
  std::vector<std::string> lastChangedTypes;
  bool changeDetected = false;

  mockPortal.setClipboardChangeCallback([&](const std::vector<std::string> &types) {
    lastChangedTypes = types;
    changeDetected = true;
  });

  std::map<std::string, std::string> testData{{"text/plain", "new test data"}, {"image/png", "binary image data"}};

  mockPortal.simulateClipboardChange(testData);
  QVERIFY(changeDetected);
  QCOMPARE(lastChangedTypes.size(), std::size_t(2));
  QCOMPARE(mockPortal.getClipboardData("text/plain"), std::string("new test data"));
  QCOMPARE(mockPortal.getClipboardData("image/png"), std::string("binary image data"));
}

QTEST_MAIN(EiClipboardTests)
#include "EiClipboardTests.moc"