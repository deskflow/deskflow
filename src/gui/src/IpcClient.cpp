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

	char codeBuf[1];
	stream.readRawData(codeBuf, 1);

	switch (codeBuf[0]) {
		case kIpcLogLine: {
			char lenBuf[1];
			stream.readRawData(lenBuf, 1);

			char* data = new char[lenBuf[0] + 1];
			stream.readRawData(data, lenBuf[0]);
			data[(int)lenBuf[0]] = 0;

			QString s(data);
			readLogLine(s);
		}
		break;
	}
}

void IpcClient::error(QAbstractSocket::SocketError error)
{
	errorMessage("ERROR: Could not connect to background service.");
}

void IpcClient::write(unsigned char code, unsigned char length, const char* data)
{
	QDataStream stream(m_Socket);

	char codeBuf[1];
	codeBuf[0] = code;
	stream.writeRawData(codeBuf, 1);

	switch (code) {
		case kIpcCommand: {
			char lenBuf[1];
			lenBuf[0] = length;
			stream.writeRawData(lenBuf, 1);
			stream.writeRawData(data, length);
		}
		break;
	}
}
