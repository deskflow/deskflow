/*
 * Deskflow -- mouse and keyboard sharing utility
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

  bool generateCertificate(const QString &path, int keyLength);
  int getCertKeyLength(const QString &path);

private:
  bool runTool(const QStringList &args);
  bool generateFingerprint(const QString &certificateFilename);

private:
  QString m_toolStdout;
};
