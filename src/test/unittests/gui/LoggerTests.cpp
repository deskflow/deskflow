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

#include "gui/Logger.h"

#include "gmock/gmock.h"
#include <gtest/gtest.h>

using namespace testing;
using namespace deskflow::gui;

TEST(LoggerTests, handleMessage_withDebugEnvVarOn_emitsNewLine)
{
  Logger logger;
  std::string newLineEmitted;
  QObject::connect(
      &logger, &Logger::newLine, //
      [&newLineEmitted](const QString &line) { newLineEmitted = line.toStdString(); }
  );
  qputenv("DESKFLOW_GUI_DEBUG", "true");
  logger.loadEnvVars();

  logger.handleMessage(QtDebugMsg, "stub", "test");

  EXPECT_THAT(newLineEmitted, HasSubstr("test"));

  qputenv("DESKFLOW_GUI_DEBUG", "");
}

TEST(LoggerTests, handleMessage_withDebugEnvVarOff_doesNotEmitNewLine)
{
  Logger logger;
  bool newLineEmitted = false;
  QObject::connect(
      &logger, &Logger::newLine, //
      [&newLineEmitted](const QString &line) { newLineEmitted = true; }
  );
  qputenv("DESKFLOW_GUI_DEBUG", "false");
  logger.loadEnvVars();

  logger.handleMessage(QtDebugMsg, "stub", "test");

  EXPECT_FALSE(newLineEmitted);

  qputenv("DESKFLOW_GUI_DEBUG", "");
}
