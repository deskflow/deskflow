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

#include <QtCore/QObject>

#include "ZeroconfRecord.h"

class QSocketNotifier;

// Bonjour flags
#define _MSL_STDINT_H
#include <stdint.h>
#if defined(Q_OS_WIN)
#define WIN32_LEAN_AND_MEAN
#endif
#include <dns_sd.h>

class ZeroconfRegister : public QObject
{
	Q_OBJECT

public:
	ZeroconfRegister(QObject* parent = 0);
	~ZeroconfRegister();

	void registerService(const ZeroconfRecord& record, quint16 servicePort);
	inline ZeroconfRecord registeredRecord() const { return finalRecord; }

signals:
	void error(DNSServiceErrorType error);
	void serviceRegistered(const ZeroconfRecord& record);

private slots:
	void socketReadyRead();

private:
	static void DNSSD_API registerService(DNSServiceRef sdRef,
		DNSServiceFlags, DNSServiceErrorType errorCode, const char* name,
		const char* regtype, const char* domain, void* context);

private:
	DNSServiceRef m_DnsServiceRef;
	QSocketNotifier* m_pSocket;
	ZeroconfRecord finalRecord;
};
