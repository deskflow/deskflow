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

#include "QIpcClient.h"

#include "IpcReader.h"
#include "global/Ipc.h"

#include <QDataStream>
#include <QHostAddress>
#include <QTimer>
#include <qlogging.h>

const auto kRetryInterval = 1000;
const auto kConnectTimeout = 5000;

QIpcClient::QIpcClient(const StreamProvider &streamProvider)
    : m_streamProvider(streamProvider) {

  m_pSocket = new QTcpSocket(this);

  if (!m_streamProvider) {
    m_streamProvider = [this]() {
      return std::make_shared<QDataStreamProxy>(m_pSocket);
    };
  }

  connect(
      m_pSocket, &QTcpSocket::connected, this, &QIpcClient::onSocketConnected);
  connect(
      m_pSocket, &QTcpSocket::errorOccurred, this, &QIpcClient::onSocketError);

  m_pReader = new IpcReader(m_pSocket);
  connect(
      m_pReader, &IpcReader::read, this, //
      &QIpcClient::onIpcReaderRead);
  connect(
      m_pReader, &IpcReader::helloBack, this,
      &QIpcClient::onIpcReaderHelloBack);
}

QIpcClient::~QIpcClient() {
  delete m_pReader;
  delete m_pSocket;
}

void QIpcClient::onSocketConnected() { sendHello(); }

void QIpcClient::connectToHost() {
  m_isConnecting = true;
  qInfo("connecting to background service...");
  const auto port = static_cast<quint16>(kIpcPort);
  m_pSocket->connectToHost(QHostAddress(QHostAddress::LocalHost), port);

  if (!m_readerStarted) {
    m_pReader->start();
    m_readerStarted = true;
  }

  QTimer::singleShot(kConnectTimeout, this, [this]() {
    if (!m_isConnected) {
      qCritical("ipc connection timeout");
    }
  });
}

void QIpcClient::disconnectFromHost() {
  m_isConnecting = false;
  qInfo("disconnected from background service");
  m_pReader->stop();
  m_pSocket->close();
  m_isConnected = false;
}

void QIpcClient::onSocketError(QAbstractSocket::SocketError socketError) {
  QString text;
  switch (socketError) {
  case 0:
    text = "connection refused";
    break;
  case 1:
    text = "remote host closed";
    break;
  default:
    text = QString("code=%1").arg(socketError);
    break;
  }

  qWarning("ipc connection error, %s", qUtf8Printable(text));

  QTimer::singleShot(kRetryInterval, this, &QIpcClient::retryConnect);
}

void QIpcClient::retryConnect() {
  if (m_isConnected) {
    qDebug("ipc already connected, skipping retry");
    return;
  } else if (!m_isConnecting) {
    qDebug("ipc not connecting, skipping retry");
    return;
  }

  qInfo("retrying connection to background service...");
  connectToHost();
}

void QIpcClient::sendHello() {
  qDebug("sending ipc hello message");
  auto stream = m_streamProvider();
  stream->writeRawData(kIpcMsgHello, 4);

  char typeBuf[1];
  typeBuf[0] = static_cast<char>(IpcClientType::GUI);
  stream->writeRawData(typeBuf, 1);
}

void QIpcClient::sendCommand(
    const QString &command, ElevateMode const elevate) {
  qDebug("sending ipc command: %s", qUtf8Printable(command));

  auto stream = m_streamProvider();
  stream->writeRawData(kIpcMsgCommand, 4);

  std::string stdStringCommand = command.toStdString();
  const char *charCommand = stdStringCommand.c_str();
  auto length = static_cast<int>(stdStringCommand.length());

  char lenBuf[4];
  intToBytes(length, lenBuf, 4);
  stream->writeRawData(lenBuf, 4);
  stream->writeRawData(charCommand, length);

  char elevateBuf[1];
  // see enum ElevateMode documentation for why this flag is mapped this way
  elevateBuf[0] = (elevate == ElevateAlways) ? 1 : 0;
  stream->writeRawData(elevateBuf, 1);
}

void QIpcClient::onIpcReaderHelloBack() {
  qDebug("ipc hello back received");
  m_isConnected = true;
  serviceReady();
}

void QIpcClient::onIpcReaderRead(const QString &text) { emit read(text); }

void QIpcClient::intToBytes(int value, char *buffer, int size) {
  if (size == 1) {
    buffer[0] = value & 0xff;
  } else if (size == 2) {
    buffer[0] = (value >> 8) & 0xff;
    buffer[1] = value & 0xff;
  } else if (size == 4) {
    buffer[0] = (value >> 24) & 0xff;
    buffer[1] = (value >> 16) & 0xff;
    buffer[2] = (value >> 8) & 0xff;
    buffer[3] = value & 0xff;
  } else {
    qFatal("intToBytes: size must be 1, 2, or 4");
  }
}
