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

#include "IpcReader.h"

#include "Logger.h"
#include "global/Ipc.h"

#include <QByteArray>
#include <QMutex>
#include <QTcpSocket>

using namespace synergy::gui;

IpcReader::IpcReader(QTcpSocket *socket) : m_Socket(socket) {}

void IpcReader::start() {
  connect(
      m_Socket, &QTcpSocket::readyRead, this, &IpcReader::onSocketReadyRead);
}

void IpcReader::stop() {
  disconnect(
      m_Socket, &QTcpSocket::readyRead, this, &IpcReader::onSocketReadyRead);
}

void IpcReader::onSocketReadyRead() {
  QMutexLocker locker(&m_Mutex);
  logVerbose("ready read");

  while (m_Socket->bytesAvailable()) {
    logVerbose("bytes available");

    char codeBuf[5];
    readStream(codeBuf, 4);
    codeBuf[4] = 0;
    logVerbose(QString("ipc read: %1").arg(codeBuf));

    if (memcmp(codeBuf, kIpcMsgLogLine, 4) == 0) {
      logVerbose("reading log line");

      char lenBuf[4];
      readStream(lenBuf, 4);
      int len = bytesToInt(lenBuf, 4);

      std::vector<char> dataBuf(len);
      readStream(dataBuf.data(), len);
      QString text = QString::fromUtf8(dataBuf.data(), len);

      emit read(text);
    } else if (memcmp(codeBuf, kIpcMsgHelloBack, 4) == 0) {
      logVerbose("reading hello back");
      emit helloBack();
    } else {
      qCritical("aborting ipc read, message invalid");
      return;
    }
  }

  logVerbose("read done");
}

bool IpcReader::readStream(char *buffer, int length) {
  logVerbose("reading stream");

  int read = 0;
  while (read < length) {
    int ask = length - read;
    if (m_Socket->bytesAvailable() < ask) {
      logVerbose("buffer too short, waiting");
      m_Socket->waitForReadyRead(-1);
    }

    int got = m_Socket->read(buffer, ask);
    read += got;

    logVerbose(QString("ask=%1 got=%2 read=%3").arg(ask).arg(got).arg(read));

    if (got == -1) {
      logVerbose("socket ended, aborting");
      return false;
    } else if (length - read > 0) {
      logVerbose(QString("more remains, seek to %1").arg(got));
      buffer += got;
    }
  }
  return true;
}

int IpcReader::bytesToInt(const char *buffer, int size) {
  if (size == 1) {
    return (unsigned char)buffer[0];
  } else if (size == 2) {
    return (((unsigned char)buffer[0]) << 8) + (unsigned char)buffer[1];
  } else if (size == 4) {
    return (((unsigned char)buffer[0]) << 24) +
           (((unsigned char)buffer[1]) << 16) +
           (((unsigned char)buffer[2]) << 8) + (unsigned char)buffer[3];
  } else {
    return 0;
  }
}
