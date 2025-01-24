/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2012 Symless Ltd.
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "IpcReader.h"

#include "Logger.h"
#include "byte_utils.h"
#include "common/ipc.h"

#include <QByteArray>
#include <QMutex>
#include <QTcpSocket>

using namespace deskflow::gui;

IpcReader::IpcReader(QTcpSocket *socket) : m_Socket(socket)
{
}

void IpcReader::start() const
{
  connect(m_Socket, &QTcpSocket::readyRead, this, &IpcReader::onSocketReadyRead);
}

void IpcReader::stop() const
{
  disconnect(m_Socket, &QTcpSocket::readyRead, this, &IpcReader::onSocketReadyRead);
}

void IpcReader::onSocketReadyRead()
{
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

      Q_EMIT read(text);
    } else if (memcmp(codeBuf, kIpcMsgHelloBack, 4) == 0) {
      logVerbose("reading hello back");
      Q_EMIT helloBack();
    } else {
      qCritical("aborting ipc read, message invalid");
      return;
    }
  }

  logVerbose("read done");
}

bool IpcReader::readStream(char *buffer, int length)
{
  logVerbose("reading stream");

  int read = 0;
  while (read < length) {
    int ask = length - read;
    if (m_Socket->bytesAvailable() < ask) {
      logVerbose("buffer too short, waiting");
      m_Socket->waitForReadyRead(-1);
    }

    auto got = m_Socket->read(buffer, ask);
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
