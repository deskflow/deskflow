/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "LogLevelTests.h"

#include <QFile>
#include <QSignalSpy>

#include "common/LogLevel.h"

void LogLevelTests::checkLogLevels_Valid()
{
  QCOMPARE(LogLevel::fromOption(QStringLiteral("Fatal")), LogLevel::Level::Fatal);
  QCOMPARE(LogLevel::fromOption(QStringLiteral("erRor")), LogLevel::Level::Error);
  QCOMPARE(LogLevel::fromOption(QStringLiteral("wArning")), LogLevel::Level::Warning);
  QCOMPARE(LogLevel::fromOption(QStringLiteral("info")), LogLevel::Level::Info);
  QCOMPARE(LogLevel::fromOption("deBug"), LogLevel::Level::Debug);
  QCOMPARE(LogLevel::fromOption("vERBOse"), LogLevel::Level::Verbose);
}

void LogLevelTests::checkLogLevels_Invalid()
{
  QCOMPARE(LogLevel::fromOption({}), LogLevel::Level::Info);
  QCOMPARE(LogLevel::fromOption(QStringLiteral("INVALID")), LogLevel::Level::Info);
  QCOMPARE(LogLevel::fromOption(QStringLiteral("deBug3")), LogLevel::Level::Info);
  QCOMPARE(LogLevel::fromOption(QStringLiteral("infomatic")), LogLevel::Level::Info);
  QCOMPARE(LogLevel::fromOption(QStringLiteral("warn")), LogLevel::Level::Info);
}

QTEST_MAIN(LogLevelTests)
