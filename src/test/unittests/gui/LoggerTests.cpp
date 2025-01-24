/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2024 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
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
