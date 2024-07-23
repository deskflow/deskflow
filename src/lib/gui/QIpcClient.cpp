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
#include "shared/Ipc.h"

#include <QDataStream>
#include <QHostAddress>
#include <QTimer>

QIpcClient::QIpcClient(const StreamProvider &streamProvider)
    : m_ReaderStarted(false),
      m_Enabled(false),
      m_StreamProvider(streamProvider) {

  m_Socket = new QTcpSocket(this);

  if (!m_StreamProvider) {
    m_StreamProvider = [this]() {
      return std::make_shared<QDataStreamProxy>(m_Socket);
    };
  }

  connect(m_Socket, SIGNAL(connected()), this, SLOT(connected()));
  connect(
      m_Socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), this,
      SLOT(error(QAbstractSocket::SocketError)));

  m_Reader = new IpcReader(m_Socket);
  connect(
      m_Reader, SIGNAL(readLogLine(const QString &)), this,
      SLOT(handleReadLogLine(const QString &)));
}

QIpcClient::~QIpcClient() {
  delete m_Reader;
  delete m_Socket;
}

void QIpcClient::connected() {

  sendHello();
  infoMessage("connection established");
}

void QIpcClient::connectToHost() {
  m_Enabled = true;

  infoMessage("connecting to service...");
  const auto port = static_cast<quint16>(kIpcPort);
  m_Socket->connectToHost(QHostAddress(QHostAddress::LocalHost), port);

  if (!m_ReaderStarted) {
    m_Reader->start();
    m_ReaderStarted = true;
  }
}

void QIpcClient::disconnectFromHost() {
  infoMessage("service disconnect");
  m_Reader->stop();
  m_Socket->close();
}

void QIpcClient::error(QAbstractSocket::SocketError error) {
  QString text;
  switch (error) {
  case 0:
    text = "connection refused";
    break;
  case 1:
    text = "remote host closed";
    break;
  default:
    text = QString("code=%1").arg(error);
    break;
  }

  errorMessage(QString("ipc connection error, %1").arg(text));

  QTimer::singleShot(1000, this, SLOT(retryConnect()));
}

void QIpcClient::retryConnect() {
  if (m_Enabled) {
    connectToHost();
  }
}

void QIpcClient::sendHello() {
  auto stream = m_StreamProvider();
  stream->writeRawData(kIpcMsgHello, 4);

  char typeBuf[1];
  typeBuf[0] = static_cast<char>(IpcClientType::GUI);
  stream->writeRawData(typeBuf, 1);
}

void QIpcClient::sendCommand(
    const QString &command, ElevateMode const elevate) {
  auto stream = m_StreamProvider();
  stream->writeRawData(kIpcMsgCommand, 4);

  std::string stdStringCommand = command.toStdString();
  const char *charCommand = stdStringCommand.c_str();
  auto length = static_cast<int>(stdStringCommand.length());

  char lenBuf[4];
  intToBytes(length, lenBuf, 4);
  stream->writeRawData(lenBuf, 4);
  stream->writeRawData(charCommand, length);

  char elevateBuf[1];
  // Refer to enum ElevateMode documentation for why this flag is mapped this
  // way
  elevateBuf[0] = (elevate == ElevateAlways) ? 1 : 0;
  stream->writeRawData(elevateBuf, 1);
}

void QIpcClient::handleReadLogLine(const QString &text) { readLogLine(text); }

// TODO: qt must have a built in way of converting int to bytes.
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
    // TODO: other sizes, if needed.
  }
}
