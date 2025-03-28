/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2021 Barrier Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "FingerprintDatabase.h"

#include <QFile>
#include <QTextStream>

void FingerprintDatabase::read(const QString &path)
{
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
    return;
  QTextStream in(&file);
  readStream(in);
}

void FingerprintDatabase::readStream(QTextStream &in)
{
  // Make sure the stream has something to read
  if (!in.device() && !in.string())
    return;

  if (in.device()) {
    if (!in.device()->isReadable())
      return;
  }

  if (in.string()) {
    if (in.string()->isEmpty())
      return;
  }

  QString line;
  while (!in.atEnd()) {
    line = in.readLine();
    if (line.isEmpty())
      continue;
    auto fingerprint = Fingerprint::fromDbLine(line);
    if (!fingerprint.isValid()) {
      continue;
    }
    m_fingerprints.append(fingerprint);
  }
}

void FingerprintDatabase::write(const QString &path)
{
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly))
    return;
  QTextStream out(&file);
  writeStream(out);
}

void FingerprintDatabase::writeStream(QTextStream &out)
{
  // Make sure the stream has somewhere to write
  if (!out.device() && !out.string())
    return;

  if (out.device()) {
    if (!out.device()->isWritable())
      return;
  }

  for (const auto &fingerprint : std::as_const(m_fingerprints)) {
    out << fingerprint.toDbLine() << "\n";
  }
}

void FingerprintDatabase::clear()
{
  m_fingerprints.clear();
}

void FingerprintDatabase::addTrusted(const Fingerprint &fingerprint)
{
  if (isTrusted(fingerprint)) {
    return;
  }
  m_fingerprints.append(fingerprint);
}

bool FingerprintDatabase::isTrusted(const Fingerprint &fingerprint)
{
  return m_fingerprints.contains(fingerprint);
}
