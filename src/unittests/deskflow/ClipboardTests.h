/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/Log.h"

#include <QTest>

class ClipboardTests : public QObject
{
  Q_OBJECT
private slots:
  // Test are run in order top to bottom
  void initTestCase();
  void basicFunction();
  void basicText();
  void textSize285();
  void htmlText();
  void dualText();
  void marshalText();
  void unMarshalText();
  void unMarshalText285();
  void unMarshalTextAndHtml();
  void equalClipboards();

private:
  const std::string kTestString1 = "deskflow rocks";
  const std::string kTestString2 = "String 020";
  Arch m_arch;
  Log m_log;
};
