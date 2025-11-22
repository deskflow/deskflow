/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Deskflow Developers
 * SPDX-FileCopyrightText: (C) 2015 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QObject>

class TlsCertificate : public QObject
{
  Q_OBJECT

public:
  explicit TlsCertificate(QObject *parent = nullptr);

  bool generateCertificate(const QString &path, int keyLength) const;
  bool generateFingerprint(const QString &certificateFilename) const;
  int getCertKeyLength(const QString &path) const;
};
