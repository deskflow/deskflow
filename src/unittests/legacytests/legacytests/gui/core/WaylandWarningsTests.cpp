/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "gui/core/WaylandWarnings.h"

#include "gui/core/CoreProcess.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace deskflow::gui;
using namespace deskflow::gui::core;

namespace {
struct MockDeps : public WaylandWarnings::Deps
{
  MOCK_METHOD(void, showWaylandLibraryError, (QWidget *), (override));
};

} // namespace

TEST(WaylandWarningsTests, showOnce_serverNoEi_showLibraryError)
{
  const auto deps = std::make_shared<MockDeps>();
  const bool hasEi = false;
  const bool hasPortal = false;
  const bool hasPortalIC = false;
  WaylandWarnings waylandWarnings(deps);
  Settings::setValue(Settings::Core::CoreMode, Settings::CoreMode::Server);

  EXPECT_CALL(*deps, showWaylandLibraryError(nullptr)).Times(1);

  waylandWarnings.showOnce(nullptr, hasEi, hasPortal, hasPortalIC);
}

TEST(WaylandWarningsTests, showOnce_serverNoPortal_showLibraryError)
{
  const auto deps = std::make_shared<MockDeps>();
  const bool hasEi = true;
  const bool hasPortal = false;
  const bool hasPortalIC = false;
  WaylandWarnings waylandWarnings(deps);
  Settings::setValue(Settings::Core::CoreMode, Settings::CoreMode::Server);

  EXPECT_CALL(*deps, showWaylandLibraryError(nullptr)).Times(1);

  waylandWarnings.showOnce(nullptr, hasEi, hasPortal, hasPortalIC);
}

TEST(WaylandWarningsTests, showOnce_serverNoPortalIc_showLibraryError)
{
  const auto deps = std::make_shared<MockDeps>();
  const bool hasEi = true;
  const bool hasPortal = true;
  const bool hasPortalIC = false;
  WaylandWarnings waylandWarnings(deps);
  Settings::setValue(Settings::Core::CoreMode, Settings::CoreMode::Server);

  EXPECT_CALL(*deps, showWaylandLibraryError(nullptr)).Times(1);

  waylandWarnings.showOnce(nullptr, hasEi, hasPortal, hasPortalIC);
}

TEST(WaylandWarningsTests, showOnce_failureCalledTwice_messageOnlyShownOnce)
{
  const auto deps = std::make_shared<MockDeps>();
  const bool hasEi = false;
  const bool hasPortal = false;
  const bool hasPortalIC = false;
  WaylandWarnings waylandWarnings(deps);
  Settings::setValue(Settings::Core::CoreMode, Settings::CoreMode::Server);

  EXPECT_CALL(*deps, showWaylandLibraryError(nullptr)).Times(1);

  waylandWarnings.showOnce(nullptr, hasEi, hasPortal, hasPortalIC);
  waylandWarnings.showOnce(nullptr, hasEi, hasPortal, hasPortalIC);
}
