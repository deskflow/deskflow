/*
 * Deskflow -- mouse and keyboard sharing utility
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
