/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2024 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "LogTests.h"
#include <clocale>
#include <iostream>
#include <sstream>

#define LEVEL_PRINT "%z\057"
#define LEVEL_ERR "%z\061"
#define LEVEL_INFO "%z\063"

QString sanitizeBuffer(const std::stringstream &in)
{
  static QRegularExpression timestampRegex("\\[\\S+\\] ");
  QString rtn = QString::fromStdString(in.str()).simplified();
  rtn.remove(timestampRegex);
  return rtn;
}

namespace {
// Side-effecting log argument used to observe whether the LOG_* macros evaluate
// their arguments. If the macro is properly level-gated, a filtered-out message
// must not call this at all.
int s_probeCalls = 0;
const char *probeArg()
{
  ++s_probeCalls;
  return "x";
}
} // namespace

void LogTests::initTestCase()
{
  std::setlocale(LC_NUMERIC, "C");
  m_log.setFilter(LogLevel::Level::Debug);
}

void LogTests::printWithErrorValidOutput()
{
  std::stringstream buffer;
  std::streambuf *old = std::cerr.rdbuf(buffer.rdbuf());

  m_log.print(nullptr, 0, LEVEL_ERR "test message");

  auto string = sanitizeBuffer(buffer);
  std::cerr.rdbuf(old);

  QCOMPARE(string, "ERROR: test message");
}

void LogTests::printTestPrintLevel()
{
  std::stringstream buffer;
  std::streambuf *old = std::cout.rdbuf(buffer.rdbuf());

  m_log.print(nullptr, 0, LEVEL_PRINT "test message");

  auto string = sanitizeBuffer(buffer);
  std::cout.rdbuf(old);

  QCOMPARE(string, "test message");
}

void LogTests::printTestWithArgs()
{
  std::stringstream buffer;
  std::streambuf *old = std::cout.rdbuf(buffer.rdbuf());

  m_log.print(nullptr, 0, LEVEL_INFO "test %s", "IamARG");

  auto string = sanitizeBuffer(buffer);
  std::cout.rdbuf(old);

  QCOMPARE(string, "INFO: test IamARG");
}

void LogTests::printTestLogString()
{
  std::stringstream buffer;
  std::streambuf *old = std::cout.rdbuf(buffer.rdbuf());

  auto longString = QString(10000, 'a');
  m_log.print(nullptr, 0, LEVEL_INFO "%s", qPrintable(longString));

  auto string = sanitizeBuffer(buffer);
  std::cout.rdbuf(old);

  QCOMPARE(string, QString("INFO: %1").arg(longString));
}

void LogTests::printLevelToHigh()
{
  std::stringstream buffer;
  std::streambuf *old = std::cout.rdbuf(buffer.rdbuf());

  m_log.print(CLOG_VERBOSE "test message");

  auto string = sanitizeBuffer(buffer);
  std::cout.rdbuf(old);

  QCOMPARE(string, QString{});
}

void LogTests::verboseArgsNotEvaluatedWhenFiltered()
{
  std::stringstream buffer;
  std::streambuf *old = std::cout.rdbuf(buffer.rdbuf());

  // Filter is Debug (from initTestCase), so a Verbose message is discarded and
  // the LOG_VERBOSE macro must NOT evaluate its argument.
  s_probeCalls = 0;
  LOG_VERBOSE("%s", probeArg());
  QCOMPARE(s_probeCalls, 0);

  // Once the filter admits Verbose, the argument is evaluated exactly once.
  m_log.setFilter(LogLevel::Level::Verbose);
  LOG_VERBOSE("%s", probeArg());
  QCOMPARE(s_probeCalls, 1);

  // Restore the filter for subsequent ordered tests.
  m_log.setFilter(LogLevel::Level::Debug);
  std::cout.rdbuf(old);
}

void LogTests::printInfoWithFileAndLine()
{
  std::stringstream buffer;
  std::streambuf *old = std::cout.rdbuf(buffer.rdbuf());

  m_log.print("test file", 123, LEVEL_INFO "test message");

  auto string = sanitizeBuffer(buffer);
  std::cout.rdbuf(old);

  QCOMPARE(string, "INFO: test message test file:123");
}

void LogTests::printErrWithFileAndLine()
{
  std::stringstream buffer;
  std::streambuf *old = std::cerr.rdbuf(buffer.rdbuf());

  m_log.print("test file", 123, LEVEL_ERR "test message");

  auto string = sanitizeBuffer(buffer);
  std::cerr.rdbuf(old);

  QCOMPARE(string, "ERROR: test message test file:123");
}

QTEST_MAIN(LogTests)
