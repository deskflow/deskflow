/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Synergy App Ltd
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "base/LogOutputters.h"

#include <QObject>
#include <QString>
#include <QTemporaryDir>

class FileLogOutputterTests : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void init();
  void write_multipleMessages_appendsEachLine();
  void write_sizeLimitExceeded_preservesPreviousLog();
  void write_rotatedLogExists_replacesIt();

private:
  QString rotatedFileName() const;
  void growLogPastSizeLimit(FileLogOutputter &outputter, const QString &marker);
  static QString readAll(const QString &fileName);

  static constexpr int s_paddingSize = 64 * 1024;
  static constexpr int s_maxPaddingWrites = 64;

  QTemporaryDir m_dir;
  QString m_fileName;
};
