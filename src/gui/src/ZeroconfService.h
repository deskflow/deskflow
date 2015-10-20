/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Synergy Si Ltd.
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
 * 
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the OpenSSL
 * library.
 * You must obey the GNU General Public License in all respects for all of
 * the code used other than OpenSSL. If you modify file(s) with this
 * exception, you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete
 * this exception statement from your version. If you delete this exception
 * statement from all source files in the program, then also delete it here.
 */

#pragma once

#include "ZeroconfServer.h"
#include "ZeroconfRecord.h"

#include <QtCore/QObject>

typedef int32_t  DNSServiceErrorType;

class ZeroconfRegister;
class ZeroconfBrowser;
class MainWindow;

class ZeroconfService : public QObject
{
	Q_OBJECT

public:
	ZeroconfService(MainWindow* mainWindow);
	~ZeroconfService();

private slots:
	void serverDetected(const QList<ZeroconfRecord>& list);
	void clientDetected(const QList<ZeroconfRecord>& list);
	void errorHandle(DNSServiceErrorType errorCode);

private:
	QString getLocalIPAddresses();
	bool registerService(bool server);

private:
	MainWindow* m_pMainWindow;
	ZeroconfServer m_zeroconfServer;
	ZeroconfBrowser* m_pZeroconfBrowser;
	ZeroconfRegister* m_pZeroconfRegister;
	bool m_ServiceRegistered;

	static const char* m_ServerServiceName;
	static const char* m_ClientServiceName;
};
