/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2015 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
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
