/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2024 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gui/core/CoreTool.h"
#include "shared/gui/TestQtCoreApp.h"

#include <gtest/gtest.h>

TEST(CoreToolTests, getProfileDir_noMock_returnsNonEmpty)
{
  TestQtCoreApp app;
  CoreTool coreTool;

  QString profileDir = coreTool.getProfileDir();

  EXPECT_FALSE(profileDir.isEmpty());
}

TEST(CoreToolTests, getInstalledDir_noMock_returnsNonEmpty)
{
  TestQtCoreApp app;
  CoreTool coreTool;

  QString installedDir = coreTool.getInstalledDir();

  EXPECT_FALSE(installedDir.isEmpty());
}

TEST(CoreToolTests, getArch_noMock_returnsNonEmpty)
{
  TestQtCoreApp app;
  CoreTool coreTool;

  QString arch = coreTool.getArch();

  EXPECT_FALSE(arch.isEmpty());
}
