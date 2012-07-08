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

#include "IpcReader.h"
#include <QTcpSocket>
#include "Ipc.h"
#include <iostream>
#include <QMutex>

IpcReader::IpcReader(QTcpSocket* socket) :
m_Socket(socket),
m_ReadyRead(false)
{
	connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()));
}

IpcReader::~IpcReader()
{
}

void IpcReader::readyRead()
{
	std::cout << "ready read" << std::endl;
	m_ReadyRead = true;
}

void IpcReader::run()
{
	m_Socket->waitForConnected(-1);
	while (true) {

		char codeBuf[1];
		readStream(codeBuf, 1);

		switch (codeBuf[0]) {
			case kIpcLogLine: {
				char lenBuf[4];
				readStream(lenBuf, 4);
				int len = bytesToInt(lenBuf, 4);

				char* data = new char[len];
				readStream(data, len);

				QString line = QString::fromUtf8(data, len);
				readLogLine(line);
				break;
			}

			default:
				std::cerr << "aborting, message invalid: " << (unsigned int)codeBuf[0] << std::endl;
				return;
		}
	}
}

void IpcReader::readStream(char* buffer, int length)
{
	QDataStream stream(m_Socket);
	std::cout << "reading stream" << std::endl;

	int read = 0;
	while (read < length) {
		int ask = length - read;
		int got = stream.readRawData(buffer, ask);

		if (got == 0) {
			std::cout << "end of buffer, waiting" << std::endl;

			// i'd love nothing more than to use a wait condition here, but
			// qt is such a fucker with mutexes (can't lock/unlock between
			// threads?! wtf?!). i'd just rather not go there (patches welcome).
			while (!m_ReadyRead) {
				QThread::usleep(50);
			}
			m_ReadyRead = false;
		}
		else if (got == -1) {
			std::cout << "socket ended, aborting" << std::endl;
			return;
		}
		else {
			read += got;
			buffer += got;

			std::cout << "> ask=" << ask << " got=" << got
				<< " read=" << read << std::endl;

			if (length - read > 0) {
				std::cout << "more remains" << std::endl;
			}
		}
	}
}

// TODO: qt must have a built in way of converting bytes to int.
int IpcReader::bytesToInt(const char *buffer, int size)
{
	if (size == 2) {
		return
			(((unsigned char)buffer[0]) << 8) +
			  (unsigned char)buffer[1];
	}
	else if (size == 4) {
		return
			(((unsigned char)buffer[0]) << 24) +
			(((unsigned char)buffer[1]) << 16) +
			(((unsigned char)buffer[2]) << 8) +
			  (unsigned char)buffer[3];
	}
	else {
		// TODO: other sizes, if needed.
		return 0;
	}
}
