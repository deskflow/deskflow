/*
 * synergy -- mouse and keyboard sharing utility
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

#include "gui/CoreInterface.h"
#include "shared/gui/QtCoreTest.h"

#include <gtest/gtest.h>

class CoreInterfaceTests : public QtCoreTest {};

TEST_F(CoreInterfaceTests, getProfileDir_noMock_returnsNonEmpty) {
  CoreInterface coreInterface;

  QString profileDir = coreInterface.getProfileDir();

  EXPECT_FALSE(profileDir.isEmpty());
}

TEST_F(CoreInterfaceTests, getInstalledDir_noMock_returnsNonEmpty) {
  CoreInterface coreInterface;

  QString installedDir = coreInterface.getInstalledDir();

  EXPECT_FALSE(installedDir.isEmpty());
}

TEST_F(CoreInterfaceTests, getArch_noMock_returnsNonEmpty) {
  CoreInterface coreInterface;

  QString arch = coreInterface.getArch();

  EXPECT_FALSE(arch.isEmpty());
}

TEST_F(CoreInterfaceTests, getSerialKeyFilePath_noMock_returnsNonEmpty) {
  CoreInterface coreInterface;

  QString serialKeyFilePath = coreInterface.getSerialKeyFilePath();

  EXPECT_FALSE(serialKeyFilePath.isEmpty());
}
