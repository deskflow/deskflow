/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/Log.h"
#include <QTemporaryDir>
#include <QTest>

class WlClipboardCompatTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void initTestCase();
  void init();
  void cleanupTestCase();
  void requiresOptIn();
  void missingTools();
  void readsConsecutiveCopies();
  void writesText_data();
  void writesText();
  void failedRead();
  void failedWrite();
  void timedOutRead();
  void stopsMonitoring();

private:
  QByteArray readFixture(const QString &name) const;
  void writeFixture(const QString &name, const QByteArray &data);
  QTemporaryDir m_directory;
  QByteArray m_originalPath;
  QByteArray m_originalOptIn;
  Log m_log;
};
