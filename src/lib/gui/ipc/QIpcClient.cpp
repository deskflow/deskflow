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

#include "QIpcClient.h"

#include "IpcReader.h"
#include "byte_utils.h"
#include "common/ipc.h"

#include <QDataStream>
#include <QHostAddress>
#include <QTimer>
#include <memory>

const auto kRetryInterval = 1000;
const auto kConnectTimeout = 5000;

using namespace deskflow::gui;

QIpcClient::QIpcClient(const StreamProvider &streamProvider) : m_streamProvider(streamProvider)
{

  m_pSocket = std::make_unique<QTcpSocket>();

  if (!m_streamProvider) {
    m_streamProvider = [this]() { return std::make_shared<QDataStreamProxy>(m_pSocket.get()); };
  }

  connect(m_pSocket.get(), &QTcpSocket::connected, this, &QIpcClient::onSocketConnected);
  connect(m_pSocket.get(), &QTcpSocket::errorOccurred, this, &QIpcClient::onSocketError);

  m_pReader = std::make_unique<IpcReader>(m_pSocket.get());
  connect(
      m_pReader.get(), &IpcReader::read, this, //
      &QIpcClient::onIpcReaderRead
  );
  connect(m_pReader.get(), &IpcReader::helloBack, this, &QIpcClient::onIpcReaderHelloBack);
}

void QIpcClient::onSocketConnected() const
{
  sendHello();
}

void QIpcClient::connectToHost()
{
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

void QIpcClient::disconnectFromHost()
{
  m_pReader->stop();
  m_pSocket->flush();
  m_pSocket->close();

  m_isConnecting = false;
  m_isConnected = false;

  qInfo("disconnected from background service");
}

void QIpcClient::onSocketError(QAbstractSocket::SocketError socketError)
{
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
  m_isConnected = false;

  QTimer::singleShot(kRetryInterval, this, &QIpcClient::onRetryConnect);
}

void QIpcClient::onRetryConnect()
{
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

void QIpcClient::sendHello() const
{
  qDebug("sending ipc hello message");
  auto stream = m_streamProvider();
  stream->writeRawData(kIpcMsgHello, 4);

  char typeBuf[1];
  typeBuf[0] = static_cast<char>(IpcClientType::GUI);
  stream->writeRawData(typeBuf, 1);
}

void QIpcClient::sendCommand(const QString &command, ElevateMode const elevate) const
{
  qDebug("sending ipc command: %s", qUtf8Printable(command));

  auto stream = m_streamProvider();
  stream->writeRawData(kIpcMsgCommand, 4);

  std::string stdStringCommand = command.toStdString();
  const char *charCommand = stdStringCommand.c_str();
  auto length = static_cast<int>(stdStringCommand.length());

  QByteArray lenBuf = intToBytes(length);
  if (lenBuf.size() != 4) {
    qFatal("unexpected int buffer size: %lld", lenBuf.size());
  }
  stream->writeRawData(lenBuf, 4);
  stream->writeRawData(charCommand, length);

  char elevateBuf[1];
  // see enum ElevateMode documentation for why this flag is mapped this way
  elevateBuf[0] = (elevate == ElevateMode::kAlways) ? 1 : 0;
  stream->writeRawData(elevateBuf, 1);
}

void QIpcClient::onIpcReaderHelloBack()
{
  qDebug("ipc hello back received");

  if (m_isConnected) {
    qWarning("ipc already connected, ignoring hello back");
    return;
  }

  m_isConnected = true;
  Q_EMIT serviceReady();
}

void QIpcClient::onIpcReaderRead(const QString &text)
{
  Q_EMIT read(text);
}
