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

#include "gui/CommandProcess.h"

#include <gtest/gtest.h>

TEST(CommandProcessTests, run_commandSucceeds_returnsOutput) {
  const QString command = "echo";
  const QStringList arguments = {"Hello, World!"};
  const QString input = "";

  CommandProcess commandProcess(command, arguments, input);

  const QString actual = commandProcess.run();

  EXPECT_EQ("Hello, World!", actual.toStdString());
}
