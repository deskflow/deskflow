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

#include "gui/core/CommandProcess.h"

#include <gtest/gtest.h>

TEST(CommandProcessTests, run_commandSucceeds_returnsOutput)
{

  // it seems that on windows, you can sometimes just call echo (this worked
  // with windows 2022), but on windows 10 you can't call echo directly.
#if defined(Q_OS_WIN)
  const QString command = "cmd";
  const QStringList arguments = {"/C", "echo Hello, World!"};
  const QString input = "";
#elif defined(Q_OS_UNIX)
  const QString command = "echo";
  const QStringList arguments = {"Hello, World!"};
  const QString input = "";
#endif // Q_OS_WIN

  CommandProcess commandProcess(command, arguments, input);

  const QString actual = commandProcess.run();

  EXPECT_EQ("Hello, World!", actual.toStdString());
}
