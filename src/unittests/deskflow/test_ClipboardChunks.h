/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "../../lib/deskflow/ClipboardChunk.h"

#include <QObject>
#include <QTest>

class ClipboardChunks_Test : public QObject
{
  Q_OBJECT
private slots:
  // Test are run in order top to bottom
  void startFormatData();
  void formatDataChunk();
  void endFormatData();
};
