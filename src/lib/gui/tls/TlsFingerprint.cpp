/*
 * Deskflow -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "TlsFingerprint.h"

#include "gui/core/CoreTool.h"

#include <QDir>
#include <QTextStream>

// TODO: Reduce duplication of these strings between here and SecureSocket.cpp
static const char kDirName[] = "tls";
static const char kLocalFilename[] = "local-fingerprint";
static const char kTrustedServersFilename[] = "trusted-servers";
static const char kTrustedClientsFilename[] = "trusted-clients";

TlsFingerprint::TlsFingerprint(const QString &filename) : m_Filename(filename)
{
}

void TlsFingerprint::trust(const QString &fingerprintText, bool append) const
{
  TlsFingerprint::persistDirectory();

  QIODevice::OpenMode openMode;
  if (append) {
    openMode = QIODevice::Append;
  } else {
    openMode = QIODevice::WriteOnly;
  }

  QFile file(filePath());
  if (file.open(openMode)) {
    QTextStream out(&file);
    out << fingerprintText << "\n";
    file.close();
  }
}

bool TlsFingerprint::fileExists() const
{
  QString dirName = TlsFingerprint::directoryPath();
  if (!QDir(dirName).exists()) {
    return false;
  }

  QFile file(filePath());
  return file.exists();
}

bool TlsFingerprint::isTrusted(const QString &fingerprintText) const
{
  const QStringList list = readList();
  for (const auto &trusted : list) {
    if (trusted == fingerprintText) {
      return true;
    }
  }
  return false;
}

QStringList TlsFingerprint::readList(const int readTo) const
{
  QStringList list;

  QString dirName = TlsFingerprint::directoryPath();
  if (!QDir(dirName).exists()) {
    return list;
  }

  QFile file(filePath());

  if (file.open(QIODevice::ReadOnly)) {
    QTextStream in(&file);
    while (!in.atEnd()) {
      list.append(in.readLine());
      if (list.size() == readTo) {
        break;
      }
    }
    file.close();
  }

  return list;
}

QString TlsFingerprint::readFirst() const
{
  QStringList list = readList(1);
  return list.at(0);
}

QString TlsFingerprint::filePath() const
{
  QString dir = TlsFingerprint::directoryPath();
  return QString("%1/%2").arg(dir, m_Filename);
}

void TlsFingerprint::persistDirectory()
{
  QDir dir(TlsFingerprint::directoryPath());
  if (!dir.exists()) {
    dir.mkpath(".");
  }
}

QString TlsFingerprint::directoryPath()
{
  CoreTool coreTool;
  QString profileDir = coreTool.getProfileDir();

  return QString("%1/%2").arg(profileDir, kDirName);
}

TlsFingerprint TlsFingerprint::local()
{
  return TlsFingerprint(kLocalFilename);
}

TlsFingerprint TlsFingerprint::trustedServers()
{
  return TlsFingerprint(kTrustedServersFilename);
}

TlsFingerprint TlsFingerprint::trustedClients()
{
  return TlsFingerprint(kTrustedClientsFilename);
}
