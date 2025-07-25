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
    m_clipboard = std::make_unique<EiClipboard>();
  }

  std::unique_ptr<EiClipboard> m_clipboard;
  std::unique_ptr<deskflow::test::MockPortalScope> m_mockScope;
};

// ===== Basic tests =====

void EiClipboardTests::constructorInitializesCorrectly()
{
  createClipboard();
  QVERIFY(m_clipboard != nullptr);
  QCOMPARE(m_clipboard->getTime(), IClipboard::Time(0));
}

void EiClipboardTests::portalAvailabilityDetection()
{
  createClipboard();
  if (deskflow::platform::kHasPortal && deskflow::platform::kHasPortalClipboard) {
    QVERIFY(!m_clipboard->isPortalAvailable());
  } else {
    QVERIFY(!m_clipboard->isPortalAvailable());
  }
}

void EiClipboardTests::openCloseBasicFunctionality()
{
  createClipboard();
  QVERIFY(m_clipboard->open(123));
  QCOMPARE(m_clipboard->getTime(), IClipboard::Time(123));

  // Should not reopen while already open
  QVERIFY(!m_clipboard->open(456));
  QCOMPARE(m_clipboard->getTime(), IClipboard::Time(123));

  m_clipboard->close();

  // Should open again
  QVERIFY(m_clipboard->open(789));
  QCOMPARE(m_clipboard->getTime(), IClipboard::Time(789));
  m_clipboard->close();
}

void EiClipboardTests::addDataRequiresOpenClipboard()
{
  createClipboard();
  // Add without opening (should do nothing)
  m_clipboard->add(IClipboard::kText, "test data");
  QVERIFY(!m_clipboard->has(IClipboard::kText));

  QVERIFY(m_clipboard->open(0));
  m_clipboard->add(IClipboard::kText, "test data");
  QVERIFY(m_clipboard->has(IClipboard::kText));
  QCOMPARE(m_clipboard->get(IClipboard::kText), std::string("test data"));
  m_clipboard->close();
}

void EiClipboardTests::hasDataRequiresOpenClipboard()
{
  createClipboard();
  QVERIFY(!m_clipboard->has(IClipboard::kText));

  QVERIFY(m_clipboard->open(0));
  QVERIFY(!m_clipboard->has(IClipboard::kText));
  m_clipboard->add(IClipboard::kText, "test");
  QVERIFY(m_clipboard->has(IClipboard::kText));
  m_clipboard->close();
}

void EiClipboardTests::getDataRequiresOpenClipboard()
{
  createClipboard();
  QCOMPARE(m_clipboard->get(IClipboard::kText), std::string(""));

  QVERIFY(m_clipboard->open(0));
  QCOMPARE(m_clipboard->get(IClipboard::kText), std::string(""));
  m_clipboard->add(IClipboard::kText, "test data");
  QCOMPARE(m_clipboard->get(IClipboard::kText), std::string("test data"));
  m_clipboard->close();
}

void EiClipboardTests::emptyRequiresOpenClipboard()
{
  createClipboard();
  QVERIFY(!m_clipboard->empty());

  QVERIFY(m_clipboard->open(0));
  m_clipboard->add(IClipboard::kText, "test");
  m_clipboard->add(IClipboard::kBitmap, "bitmap data");
  QVERIFY(m_clipboard->has(IClipboard::kText));
  QVERIFY(m_clipboard->has(IClipboard::kBitmap));

  if (deskflow::platform::kHasPortal && deskflow::platform::kHasPortalClipboard) {
    QVERIFY(m_clipboard->empty());
  } else {
    QVERIFY(!m_clipboard->empty());
  }
  m_clipboard->close();
}

void EiClipboardTests::multipleFormatsSupport()
{
  createClipboard();
  QVERIFY(m_clipboard->open(0));
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
}

void EiClipboardTests::invalidFormatHandling()
{
  createClipboard();
  QVERIFY(m_clipboard->open(0));
  QVERIFY(!m_clipboard->has(static_cast<IClipboard::EFormat>(-1)));
  QVERIFY(!m_clipboard->has(static_cast<IClipboard::EFormat>(IClipboard::kNumFormats)));
  QCOMPARE(m_clipboard->get(static_cast<IClipboard::EFormat>(-1)), std::string(""));
  QCOMPARE(m_clipboard->get(static_cast<IClipboard::EFormat>(IClipboard::kNumFormats)), std::string(""));
  m_clipboard->close();
}

void EiClipboardTests::dataOverwrite()
{
  createClipboard();
  QVERIFY(m_clipboard->open(0));
  m_clipboard->add(IClipboard::kText, "initial data");
  QCOMPARE(m_clipboard->get(IClipboard::kText), std::string("initial data"));
  m_clipboard->add(IClipboard::kText, "new data");
  QCOMPARE(m_clipboard->get(IClipboard::kText), std::string("new data"));
  m_clipboard->close();
}

void EiClipboardTests::largeDataHandling()
{
  createClipboard();
  QVERIFY(m_clipboard->open(0));
  std::string largeData(1024 * 1024, 'A');
  m_clipboard->add(IClipboard::kText, largeData);
  QVERIFY(m_clipboard->has(IClipboard::kText));
  QCOMPARE(m_clipboard->get(IClipboard::kText), largeData);
  m_clipboard->close();
}

void EiClipboardTests::emptyDataHandling()
{
  createClipboard();
  QVERIFY(m_clipboard->open(0));
  m_clipboard->add(IClipboard::kText, "");
  QVERIFY(m_clipboard->has(IClipboard::kText));
  QCOMPARE(m_clipboard->get(IClipboard::kText), std::string(""));
  m_clipboard->close();
}

void EiClipboardTests::timeHandling()
{
  createClipboard();
  QVERIFY(m_clipboard->open(0));
  QCOMPARE(m_clipboard->getTime(), IClipboard::Time(0));
  m_clipboard->close();

  QVERIFY(m_clipboard->open(12345));
  QCOMPARE(m_clipboard->getTime(), IClipboard::Time(12345));
  m_clipboard->close();

  QVERIFY(m_clipboard->open(static_cast<IClipboard::Time>(-1)));
  QCOMPARE(m_clipboard->getTime(), static_cast<IClipboard::Time>(-1));
  m_clipboard->close();
}

// ===== Mock portal tests =====

void EiClipboardTests::mockPortalAvailability()
{
  m_mockScope = std::make_unique<deskflow::test::MockPortalScope>();
  if (deskflow::platform::kHasPortal && deskflow::platform::kHasPortalClipboard) {
    QVERIFY(m_mockScope->isAvailable());
  } else {
    QVERIFY(!m_mockScope->isAvailable());
  }
  m_mockScope.reset();
}

void EiClipboardTests::clipboardMonitoring()
{
  m_mockScope = std::make_unique<deskflow::test::MockPortalScope>();
  if (!m_mockScope->isAvailable()) {
    QSKIP("Mock portal not available");
  }

  createClipboard();
  QVERIFY(!m_clipboard->isMonitoring());
  if (m_clipboard->startMonitoring()) {
    QVERIFY(m_clipboard->isMonitoring());
    m_clipboard->stopMonitoring();
    QVERIFY(!m_clipboard->isMonitoring());
  }
  m_mockScope.reset();
}

void EiClipboardTests::mockClipboardData()
{
  m_mockScope = std::make_unique<deskflow::test::MockPortalScope>();
  if (!m_mockScope->isAvailable()) {
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
  m_mockScope.reset();
}

void EiClipboardTests::clipboardChangeSimulation()
{
  m_mockScope = std::make_unique<deskflow::test::MockPortalScope>();
  if (!m_mockScope->isAvailable()) {
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
  m_mockScope.reset();
}

QTEST_MAIN(EiClipboardTests)
#include "EiClipboardTests.moc"
