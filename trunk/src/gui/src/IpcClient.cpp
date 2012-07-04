/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Nick Bolton
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
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

IpcClient::IpcClient()
{
	m_Socket = new QTcpSocket(this);
	connect(m_Socket, SIGNAL(readyRead()), this, SLOT(read()));
	connect(m_Socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(error(QAbstractSocket::SocketError)));
}

IpcClient::~IpcClient()
{
}

void IpcClient::connectToHost()
{
	m_Socket->connectToHost(QHostAddress(QHostAddress::LocalHost), IPC_PORT);
}

void IpcClient::read()
{
	QDataStream stream(m_Socket);

	while (m_Socket->bytesAvailable() != 0) {

		char codeBuf[1];
		stream.readRawData(codeBuf, 1);

		switch (codeBuf[0]) {
			case kIpcLogLine: {
				// TODO: qt must have a built in way of converting bytes to int.
				char lenBuf[2];
				stream.readRawData(lenBuf, 2);
				int len = (lenBuf[0] << 8) + lenBuf[1];

				// HACK: sometimes the size is wrong (probably a bug in the above code)
				// but the following text always seems to be valid, so just read a large
				// amount from the buffer and put a \0 at the end if the number looks
				// valid (ugh, this sucks).
				char* data = new char[1024];
				stream.readRawData(data, 1024);
				if (len > -1) {
					data[len] = '\0';
				}

				QString s = QString::fromUtf8(data);
				readLogLine(s);
			}
			break;

		default: {
				std::cerr << "invalid code: " << codeBuf[0] << std::endl;
			}
			break;
		}
	}
}

void IpcClient::error(QAbstractSocket::SocketError error)
{
	errorMessage(QString("ERROR: could not connect to background service, code=%1").arg(error));
}

void IpcClient::write(unsigned char code, unsigned char length, const char* data)
{
	QDataStream stream(m_Socket);

	char codeBuf[1];
	codeBuf[0] = code;
	stream.writeRawData(codeBuf, 1);

	switch (code) {
		case kIpcCommand: {
			// TODO: qt must have a built in way of converting int to bytes.
			char lenBuf[2];
			lenBuf[0] = (length >> 8) & 0xff;
			lenBuf[1] = length & 0xff;
			stream.writeRawData(lenBuf, 2);

			stream.writeRawData(data, length);
		}
		break;
	}
}
