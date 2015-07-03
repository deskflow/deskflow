/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Synergy Si Ltd.
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

#pragma once

#include <QObject>
#include <QAbstractSocket>

class QTcpSocket;
class IpcReader;

class IpcClient : public QObject
{
	 Q_OBJECT

public:
	IpcClient();
	virtual ~IpcClient();

	void sendHello();
	void sendCommand(const QString& command, bool elevate);
	void connectToHost();
	void disconnectFromHost();

public slots:
	void retryConnect();

private:
	void intToBytes(int value, char* buffer, int size);

private slots:
	void connected();
	void error(QAbstractSocket::SocketError error);
	void handleReadLogLine(const QString& text);

signals:
	void readLogLine(const QString& text);
	void infoMessage(const QString& text);
	void errorMessage(const QString& text);

private:
	QTcpSocket* m_Socket;
	IpcReader* m_Reader;
	bool m_ReaderStarted;
	bool m_Enabled;
};
