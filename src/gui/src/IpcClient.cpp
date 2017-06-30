/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2012 Nick Bolton
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

#include "IpcClient.h"
#include <QTcpSocket>
#include <QHostAddress>
#include <iostream>
#include <QTimer>
#include "IpcReader.h"
#include "Ipc.h"
#include <QDataStream>

IpcClient::IpcClient () : m_ReaderStarted (false), m_Enabled (false) {
    m_Socket = new QTcpSocket (this);
    connect (m_Socket, SIGNAL (connected ()), this, SLOT (connected ()));
    connect (m_Socket,
             SIGNAL (error (QAbstractSocket::SocketError)),
             this,
             SLOT (error (QAbstractSocket::SocketError)));

    m_Reader = new IpcReader (m_Socket);
    connect (m_Reader,
             SIGNAL (readLogLine (const QString&)),
             this,
             SLOT (handleReadLogLine (const QString&)));
}

IpcClient::~IpcClient () {
}

void
IpcClient::connected () {
    sendHello ();
    infoMessage ("connection established");
}

void
IpcClient::connectToHost () {
    m_Enabled = true;

    infoMessage ("connecting to service...");
    m_Socket->connectToHost (QHostAddress (QHostAddress::LocalHost), IPC_PORT);

    if (!m_ReaderStarted) {
        m_Reader->start ();
        m_ReaderStarted = true;
    }
}

void
IpcClient::disconnectFromHost () {
    infoMessage ("service disconnect");
    m_Reader->stop ();
    m_Socket->close ();
}

void
IpcClient::error (QAbstractSocket::SocketError error) {
    QString text;
    switch (error) {
        case 0:
            text = "connection refused";
            break;
        case 1:
            text = "remote host closed";
            break;
        default:
            text = QString ("code=%1").arg (error);
            break;
    }

    errorMessage (QString ("ipc connection error, %1").arg (text));

    QTimer::singleShot (1000, this, SLOT (retryConnect ()));
}

void
IpcClient::retryConnect () {
    if (m_Enabled) {
        connectToHost ();
    }
}

void
IpcClient::sendHello () {
    QDataStream stream (m_Socket);
    stream.writeRawData (kIpcMsgHello, 4);

    char typeBuf[1];
    typeBuf[0] = kIpcClientGui;
    stream.writeRawData (typeBuf, 1);
}

void
IpcClient::sendCommand (const QString& command, ElevateMode const elevate) {
    QDataStream stream (m_Socket);

    stream.writeRawData (kIpcMsgCommand, 4);

    std::string stdStringCommand = command.toStdString ();
    const char* charCommand      = stdStringCommand.c_str ();
    int length                   = strlen (charCommand);

    char lenBuf[4];
    intToBytes (length, lenBuf, 4);
    stream.writeRawData (lenBuf, 4);
    stream.writeRawData (charCommand, length);

    char elevateBuf[1];
    // Refer to enum ElevateMode documentation for why this flag is mapped this
    // way
    elevateBuf[0] = (elevate == ElevateAlways) ? 1 : 0;
    stream.writeRawData (elevateBuf, 1);
}

void
IpcClient::handleReadLogLine (const QString& text) {
    readLogLine (text);
}

// TODO: qt must have a built in way of converting int to bytes.
void
IpcClient::intToBytes (int value, char* buffer, int size) {
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
