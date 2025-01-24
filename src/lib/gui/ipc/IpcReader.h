/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QMutex>
#include <QObject>

class QTcpSocket;

class IpcReader : public QObject
{
  Q_OBJECT;

public:
  explicit IpcReader(QTcpSocket *socket);
  ~IpcReader() override = default;
  void start() const;
  void stop() const;

signals:
  void read(const QString &text);
  void helloBack();

private:
  bool readStream(char *buffer, int length);

private slots:
  void onSocketReadyRead();

private:
  QTcpSocket *m_Socket;
  QMutex m_Mutex;
};
