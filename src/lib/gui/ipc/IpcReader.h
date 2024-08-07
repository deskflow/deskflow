/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Symless Ltd.
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

#pragma once

#include <QMutex>
#include <QObject>

class QTcpSocket;

class IpcReader : public QObject {
  Q_OBJECT;

public:
  IpcReader(QTcpSocket *socket);
  virtual ~IpcReader();
  void start();
  void stop();

signals:
  void read(const QString &text);
  void helloBack();

private:
  bool readStream(char *buffer, int length);
  int bytesToInt(const char *buffer, int size);

private slots:
  void onSocketReadyRead();

private:
  QTcpSocket *m_Socket;
  QMutex m_Mutex;
};
