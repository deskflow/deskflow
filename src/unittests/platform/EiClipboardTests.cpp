/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/EiClipboard.h"
#include "platform/Wayland.h"
#include "MockPortal.h"

#include <gtest/gtest.h>
#include <string>

using namespace deskflow;

class EiClipboardTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_clipboard = std::make_unique<EiClipboard>();
    }

    void TearDown() override
    {
        m_clipboard.reset();
    }

    std::unique_ptr<EiClipboard> m_clipboard;
};

TEST_F(EiClipboardTest, ConstructorInitializesCorrectly)
{
    EXPECT_NE(m_clipboard, nullptr);
    EXPECT_EQ(m_clipboard->getTime(), 0);
}

TEST_F(EiClipboardTest, PortalAvailabilityDetection)
{
    // Portal availability depends on compile-time flags
    if (deskflow::platform::kHasPortal && deskflow::platform::kHasPortalClipboard) {
        // When compiled with portal support, availability depends on runtime detection
        // For now, this will be false since the portal clipboard interface doesn't exist yet
        EXPECT_FALSE(m_clipboard->isPortalAvailable());
    } else {
        // When compiled without portal support, should always be false
        EXPECT_FALSE(m_clipboard->isPortalAvailable());
    }
}

TEST_F(EiClipboardTest, OpenCloseBasicFunctionality)
{
    EXPECT_TRUE(m_clipboard->open(123));
    EXPECT_EQ(m_clipboard->getTime(), 123);
    
    // Should not be able to open again while already open
    EXPECT_FALSE(m_clipboard->open(456));
    EXPECT_EQ(m_clipboard->getTime(), 123); // Time should not change
    
    m_clipboard->close();
    
    // Should be able to open again after closing
    EXPECT_TRUE(m_clipboard->open(789));
    EXPECT_EQ(m_clipboard->getTime(), 789);
    
    m_clipboard->close();
}

TEST_F(EiClipboardTest, AddDataRequiresOpenClipboard)
{
    // Should not be able to add data when clipboard is not open
    m_clipboard->add(IClipboard::kText, "test data");
    
    // Open clipboard and try again
    ASSERT_TRUE(m_clipboard->open(0));
    m_clipboard->add(IClipboard::kText, "test data");
    
    // Verify data was added (even if portal is not available)
    EXPECT_TRUE(m_clipboard->has(IClipboard::kText));
    EXPECT_EQ(m_clipboard->get(IClipboard::kText), "test data");
    
    m_clipboard->close();
}

TEST_F(EiClipboardTest, HasDataRequiresOpenClipboard)
{
    // Should return false when clipboard is not open
    EXPECT_FALSE(m_clipboard->has(IClipboard::kText));
    
    ASSERT_TRUE(m_clipboard->open(0));
    
    // Should return false for data that hasn't been added
    EXPECT_FALSE(m_clipboard->has(IClipboard::kText));
    
    // Add data and check again
    m_clipboard->add(IClipboard::kText, "test");
    EXPECT_TRUE(m_clipboard->has(IClipboard::kText));
    
    m_clipboard->close();
}

TEST_F(EiClipboardTest, GetDataRequiresOpenClipboard)
{
    // Should return empty string when clipboard is not open
    EXPECT_EQ(m_clipboard->get(IClipboard::kText), "");
    
    ASSERT_TRUE(m_clipboard->open(0));
    
    // Should return empty string for data that hasn't been added
    EXPECT_EQ(m_clipboard->get(IClipboard::kText), "");
    
    // Add data and retrieve it
    m_clipboard->add(IClipboard::kText, "test data");
    EXPECT_EQ(m_clipboard->get(IClipboard::kText), "test data");
    
    m_clipboard->close();
}

TEST_F(EiClipboardTest, EmptyRequiresOpenClipboard)
{
    // Should return false when clipboard is not open
    EXPECT_FALSE(m_clipboard->empty());
    
    ASSERT_TRUE(m_clipboard->open(0));
    
    // Add some data
    m_clipboard->add(IClipboard::kText, "test");
    m_clipboard->add(IClipboard::kBitmap, "bitmap data");
    EXPECT_TRUE(m_clipboard->has(IClipboard::kText));
    EXPECT_TRUE(m_clipboard->has(IClipboard::kBitmap));
    
    // Empty the clipboard
    if (deskflow::platform::kHasPortal && deskflow::platform::kHasPortalClipboard) {
        // With portal support, empty should work
        EXPECT_TRUE(m_clipboard->empty());
    } else {
        // Without portal support, empty should fail
        EXPECT_FALSE(m_clipboard->empty());
    }
    
    m_clipboard->close();
}

TEST_F(EiClipboardTest, MultipleFormatsSupport)
{
    ASSERT_TRUE(m_clipboard->open(0));
    
    // Add different types of data
    m_clipboard->add(IClipboard::kText, "plain text");
    m_clipboard->add(IClipboard::kHTML, "<html><body>HTML content</body></html>");
    m_clipboard->add(IClipboard::kBitmap, "binary bitmap data");
    
    // Verify all formats are available
    EXPECT_TRUE(m_clipboard->has(IClipboard::kText));
    EXPECT_TRUE(m_clipboard->has(IClipboard::kHTML));
    EXPECT_TRUE(m_clipboard->has(IClipboard::kBitmap));
    
    // Verify data integrity
    EXPECT_EQ(m_clipboard->get(IClipboard::kText), "plain text");
    EXPECT_EQ(m_clipboard->get(IClipboard::kHTML), "<html><body>HTML content</body></html>");
    EXPECT_EQ(m_clipboard->get(IClipboard::kBitmap), "binary bitmap data");
    
    m_clipboard->close();
}

TEST_F(EiClipboardTest, InvalidFormatHandling)
{
    ASSERT_TRUE(m_clipboard->open(0));
    
    // Test with invalid format values
    EXPECT_FALSE(m_clipboard->has(static_cast<IClipboard::EFormat>(-1)));
    EXPECT_FALSE(m_clipboard->has(static_cast<IClipboard::EFormat>(IClipboard::kNumFormats)));
    
    EXPECT_EQ(m_clipboard->get(static_cast<IClipboard::EFormat>(-1)), "");
    EXPECT_EQ(m_clipboard->get(static_cast<IClipboard::EFormat>(IClipboard::kNumFormats)), "");
    
    m_clipboard->close();
}

TEST_F(EiClipboardTest, DataOverwrite)
{
    ASSERT_TRUE(m_clipboard->open(0));
    
    // Add initial data
    m_clipboard->add(IClipboard::kText, "initial data");
    EXPECT_EQ(m_clipboard->get(IClipboard::kText), "initial data");
    
    // Overwrite with new data
    m_clipboard->add(IClipboard::kText, "new data");
    EXPECT_EQ(m_clipboard->get(IClipboard::kText), "new data");
    
    m_clipboard->close();
}

TEST_F(EiClipboardTest, LargeDataHandling)
{
    ASSERT_TRUE(m_clipboard->open(0));
    
    // Test with large data (1MB string)
    std::string largeData(1024 * 1024, 'A');
    m_clipboard->add(IClipboard::kText, largeData);
    
    EXPECT_TRUE(m_clipboard->has(IClipboard::kText));
    EXPECT_EQ(m_clipboard->get(IClipboard::kText), largeData);
    
    m_clipboard->close();
}

TEST_F(EiClipboardTest, EmptyDataHandling)
{
    ASSERT_TRUE(m_clipboard->open(0));
    
    // Test with empty data
    m_clipboard->add(IClipboard::kText, "");
    EXPECT_TRUE(m_clipboard->has(IClipboard::kText));
    EXPECT_EQ(m_clipboard->get(IClipboard::kText), "");
    
    m_clipboard->close();
}

TEST_F(EiClipboardTest, TimeHandling)
{
    // Test time values
    EXPECT_TRUE(m_clipboard->open(0));
    EXPECT_EQ(m_clipboard->getTime(), 0);
    m_clipboard->close();

    EXPECT_TRUE(m_clipboard->open(12345));
    EXPECT_EQ(m_clipboard->getTime(), 12345);
    m_clipboard->close();

    EXPECT_TRUE(m_clipboard->open(static_cast<IClipboard::Time>(-1)));
    EXPECT_EQ(m_clipboard->getTime(), static_cast<IClipboard::Time>(-1));
    m_clipboard->close();
}

// Advanced tests with mock portal
class EiClipboardMockTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_mockScope = std::make_unique<deskflow::test::MockPortalScope>();
        m_clipboard = std::make_unique<EiClipboard>();
    }

    void TearDown() override
    {
        m_clipboard.reset();
        m_mockScope.reset();
    }

    std::unique_ptr<deskflow::test::MockPortalScope> m_mockScope;
    std::unique_ptr<EiClipboard> m_clipboard;
};

TEST_F(EiClipboardMockTest, MockPortalAvailability)
{
    if (deskflow::platform::kHasPortal && deskflow::platform::kHasPortalClipboard) {
        // When compiled with portal support, mock should be available
        EXPECT_TRUE(m_mockScope->isAvailable());
    } else {
        // When compiled without portal support, mock should not be available
        EXPECT_FALSE(m_mockScope->isAvailable());
    }
}

TEST_F(EiClipboardMockTest, ClipboardMonitoring)
{
    if (!m_mockScope->isAvailable()) {
        GTEST_SKIP() << "Mock portal not available";
    }

    // Test clipboard monitoring functionality
    EXPECT_FALSE(m_clipboard->isMonitoring());

    if (m_clipboard->startMonitoring()) {
        EXPECT_TRUE(m_clipboard->isMonitoring());

        m_clipboard->stopMonitoring();
        EXPECT_FALSE(m_clipboard->isMonitoring());
    }
}

TEST_F(EiClipboardMockTest, MockClipboardData)
{
    if (!m_mockScope->isAvailable()) {
        GTEST_SKIP() << "Mock portal not available";
    }

    auto& mockPortal = m_mockScope->portal();

    // Set some test data in the mock portal
    mockPortal.setClipboardData("text/plain", "test data");
    mockPortal.setClipboardData("text/html", "<p>HTML test</p>");

    // Verify data is available
    EXPECT_EQ(mockPortal.getClipboardData("text/plain"), "test data");
    EXPECT_EQ(mockPortal.getClipboardData("text/html"), "<p>HTML test</p>");

    // Check available MIME types
    auto mimeTypes = mockPortal.getAvailableMimeTypes();
    EXPECT_EQ(mimeTypes.size(), 2);
    EXPECT_TRUE(std::find(mimeTypes.begin(), mimeTypes.end(), "text/plain") != mimeTypes.end());
    EXPECT_TRUE(std::find(mimeTypes.begin(), mimeTypes.end(), "text/html") != mimeTypes.end());
}

TEST_F(EiClipboardMockTest, ClipboardChangeSimulation)
{
    if (!m_mockScope->isAvailable()) {
        GTEST_SKIP() << "Mock portal not available";
    }

    auto& mockPortal = m_mockScope->portal();

    // Set up change tracking
    std::vector<std::string> lastChangedTypes;
    bool changeDetected = false;

    mockPortal.setClipboardChangeCallback([&](const std::vector<std::string>& types) {
        lastChangedTypes = types;
        changeDetected = true;
    });

    // Simulate clipboard change
    std::map<std::string, std::string> testData = {
        {"text/plain", "new test data"},
        {"image/png", "binary image data"}
    };

    mockPortal.simulateClipboardChange(testData);

    // Verify change was detected
    EXPECT_TRUE(changeDetected);
    EXPECT_EQ(lastChangedTypes.size(), 2);

    // Verify data is accessible
    EXPECT_EQ(mockPortal.getClipboardData("text/plain"), "new test data");
    EXPECT_EQ(mockPortal.getClipboardData("image/png"), "binary image data");
}
