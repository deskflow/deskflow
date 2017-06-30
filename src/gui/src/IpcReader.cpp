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

#include "IpcReader.h"
#include <QTcpSocket>
#include "Ipc.h"
#include <iostream>
#include <QMutex>
#include <QByteArray>

IpcReader::IpcReader (QTcpSocket* socket) : m_Socket (socket) {
}

IpcReader::~IpcReader () {
}

void
IpcReader::start () {
    connect (m_Socket, SIGNAL (readyRead ()), this, SLOT (read ()));
}

void
IpcReader::stop () {
    disconnect (m_Socket, SIGNAL (readyRead ()), this, SLOT (read ()));
}

void
IpcReader::read () {
    QMutexLocker locker (&m_Mutex);
    std::cout << "ready read" << std::endl;

    while (m_Socket->bytesAvailable ()) {
        std::cout << "bytes available" << std::endl;

        char codeBuf[5];
        readStream (codeBuf, 4);
        codeBuf[4] = 0;
        std::cout << "ipc read: " << codeBuf << std::endl;

        if (memcmp (codeBuf, kIpcMsgLogLine, 4) == 0) {
            std::cout << "reading log line" << std::endl;

            char lenBuf[4];
            readStream (lenBuf, 4);
            int len = bytesToInt (lenBuf, 4);

            char* data = new char[len];
            readStream (data, len);
            QString line = QString::fromUtf8 (data, len);
            delete[] data;

            readLogLine (line);
        } else {
            std::cerr << "aborting, message invalid" << std::endl;
            return;
        }
    }

    std::cout << "read done" << std::endl;
}

bool
IpcReader::readStream (char* buffer, int length) {
    std::cout << "reading stream" << std::endl;

    int read = 0;
    while (read < length) {
        int ask = length - read;
        if (m_Socket->bytesAvailable () < ask) {
            std::cout << "buffer too short, waiting" << std::endl;
            m_Socket->waitForReadyRead (-1);
        }

        int got = m_Socket->read (buffer, ask);
        read += got;

        std::cout << "> ask=" << ask << " got=" << got << " read=" << read
                  << std::endl;

        if (got == -1) {
            std::cout << "socket ended, aborting" << std::endl;
            return false;
        } else if (length - read > 0) {
            std::cout << "more remains, seek to " << got << std::endl;
            buffer += got;
        }
    }
    return true;
}

int
IpcReader::bytesToInt (const char* buffer, int size) {
    if (size == 1) {
        return (unsigned char) buffer[0];
    } else if (size == 2) {
        return (((unsigned char) buffer[0]) << 8) + (unsigned char) buffer[1];
    } else if (size == 4) {
        return (((unsigned char) buffer[0]) << 24) +
               (((unsigned char) buffer[1]) << 16) +
               (((unsigned char) buffer[2]) << 8) + (unsigned char) buffer[3];
    } else {
        return 0;
    }
}
