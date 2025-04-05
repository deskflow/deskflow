/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "../../lib/deskflow/ArgParser.h"
#include <QObject>

#include "base/Log.h"

class ArgParser_Test : public QObject
{
  Q_OBJECT
private slots:
  void initTestCase();
  // Test are run in order top to bottom
  void isArg();
  void missingArg();
  void withQuotes();
  void splitCommand();
  void getArgv();
  void assembleCommand();
  void serverArgs();
  void clientArgs();
  void deprecatedArg_crypoPass_true();
  void deprecatedArg_crypoPass_false();

private:
  Arch m_arch;
  Log m_log;
  ArgParser m_parser = ArgParser(nullptr);
};
