/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/Log.h"

#include <QTest>

class ConfigTests : public QObject
{
  Q_OBJECT
private slots:
  // Test are run in order top to bottom
  void initTestCase();
  void loadFile();
  void load_EmptyFile();
  void load_NonExsitingFile();
  void load_InvalidConfig();
  void load_missingSections();
  void load_badTable();
  void load_lastArg();
  void load_noArgs();

private:
  Arch m_arch;
  Log m_log;
};
