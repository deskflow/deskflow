/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "gui/core/CoreTool.h"
#include "shared/gui/TestQtCoreApp.h"

#include <gtest/gtest.h>

TEST(CoreToolTests, getInstalledDir_noMock_returnsNonEmpty)
{
  TestQtCoreApp app;
  CoreTool coreTool;

  QString installedDir = coreTool.getInstalledDir();

  EXPECT_FALSE(installedDir.isEmpty());
}
