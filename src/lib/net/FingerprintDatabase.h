/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2021 Barrier Contributors
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "Fingerprint.h"

#include <QList>

class FingerprintDatabase
{
public:
  void read(const QString &path);
  void write(const QString &path);

  void readStream(QTextStream &in);
  void writeStream(QTextStream &out);

  void clear();
  void addTrusted(const Fingerprint &fingerprint);
  bool isTrusted(const Fingerprint &fingerprint);

  const QList<Fingerprint> &fingerprints() const
  {
    return m_fingerprints;
  }

private:
  QList<Fingerprint> m_fingerprints;
};
