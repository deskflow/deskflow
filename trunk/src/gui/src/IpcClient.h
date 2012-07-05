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

#pragma once

#include <QObject>
#include <QAbstractSocket>

#define IPC_PORT 24801

class QTcpSocket;

class IpcClient : public QObject
{
	 Q_OBJECT

public:
    IpcClient();
	virtual ~IpcClient();

	void write(unsigned char code, unsigned char length, const char* data);

public slots:
	void connectToHost();

private:
	int bytesToInt(const char* buffer, int size);
	void intToBytes(int value, char* buffer, int size);

private slots:
	void connected();
	void read();
	void error(QAbstractSocket::SocketError error);

signals:
	void readLogLine(const QString& text);
	void infoMessage(const QString& text);
	void errorMessage(const QString& text);

private:
	QTcpSocket* m_Socket;
};

enum IpcMessageType {
	kIpcHello,
	kIpcLogLine,
	kIpcCommand,
	kIpcShutdown,
};

enum IpcClientType {
	kIpcClientUnknown,
	kIpcClientGui,
	kIpcClientNode,
};
