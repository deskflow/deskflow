/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "FileTail.h"

#include <QDebug>
#include <QFile>
#include <QFileSystemWatcher>
#include <QObject>
#include <QTextStream>

namespace deskflow::gui {

FileTail::FileTail(const QString &filePath, QObject *parent)
    : QObject(parent),
      m_file(filePath),
      m_watcher(new QFileSystemWatcher(this))
{
  if (!m_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qCritical() << "failed to open file for tail:" << filePath;
    return;
  }

  qDebug() << "starting file tail:" << filePath;
  m_watcher->addPath(filePath);
  m_lastPos = m_file.size();

  connect(m_watcher, &QFileSystemWatcher::fileChanged, this, &FileTail::handleFileChanged);
}

void FileTail::handleFileChanged(const QString &)
{
  m_file.seek(m_lastPos);
  QTextStream stream(&m_file);
  while (!stream.atEnd()) {
    QString line = stream.readLine();
    Q_EMIT newLine(line);
  }
  m_lastPos = m_file.pos();
}

} // namespace deskflow::gui
