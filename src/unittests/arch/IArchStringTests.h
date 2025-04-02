/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "base/Log.h"

#include <QTest>

class SampleIArchString : public IArchString
{
public:
  EWideCharEncoding getWideCharEncoding() override
  {
    return kUTF16;
  }
};

class IArchStringTests : public QObject
{
  Q_OBJECT
private slots:
  void initTestCase();
  // Test are run in order top to bottom
  void convertStringWCToMB_buffer();
  void convertStringWCToMB_noBuffer();
  void convertStringMBToWC();

private:
  Arch m_arch;
  Log m_log;
};
