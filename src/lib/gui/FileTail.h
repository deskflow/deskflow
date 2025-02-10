/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QFile>
#include <QObject>

class QFileSystemWatcher;

namespace deskflow::gui {

class FileTail : public QObject
{
  Q_OBJECT

public:
  FileTail(const QString &filePath, QObject *parent = nullptr);

signals:
  void newLine(const QString &line);

private slots:
  void handleFileChanged(const QString &);

private:
  QFile m_file;
  QFileSystemWatcher *m_watcher = nullptr;
  qint64 m_lastPos;
};

} // namespace deskflow::gui
