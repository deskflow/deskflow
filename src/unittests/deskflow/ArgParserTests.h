/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/Log.h"
#include "deskflow/ArgParser.h"

#include <QTest>

class ArgParserTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase();
  // Test are run in order top to bottom
  void isArg();
  void missingArg();
  void withQuotes();
  void splitCommand();
  void getArgv();
  void assembleCommand();
  void serverArgs();
  void server_setConfigFile();
  void server_unexpectedParam();
  void clientArgs();
  void client_yScroll();
  void client_setLangSync();
  void client_setInvertScroll();
  void client_commonArgs();
  void client_setAddress();
  void client_badArgs();
  void deprecatedArg_crypoPass_true();
  void deprecatedArg_crypoPass_false();
  void generic_logFile();
  void generic_logFileWithSpace();
  void generic_noRestart();
  void generic_restart();
  void generic_unknown();
  void generic_noHook();

private:
  Arch m_arch;
  Log m_log;
  ArgParser m_parser = ArgParser(nullptr);
};
