/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include <QObject>

#include "base/Log.h"

class OSXClipboard_Test : public QObject
{
  Q_OBJECT
private slots:
  // Test are run in order top to bottom
  void open();
  void singleFormat();
  void formatConvert_UTF8();

private:
  Arch m_arch;
  Log m_log;
  const std::string m_testString = "deskflow test string";
};
