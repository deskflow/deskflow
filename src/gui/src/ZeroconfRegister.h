/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2016 Symless Ltd.
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

class ZeroconfRegister : public QObject {
    Q_OBJECT

public:
    ZeroconfRegister (QObject* parent = 0);
    ~ZeroconfRegister ();

    void registerService (const ZeroconfRecord& record, quint16 servicePort);
    inline ZeroconfRecord
    registeredRecord () const {
        return finalRecord;
    }

signals:
    void error (DNSServiceErrorType error);
    void serviceRegistered (const ZeroconfRecord& record);

private slots:
    void socketReadyRead ();

private:
    static void DNSSD_API registerService (DNSServiceRef sdRef, DNSServiceFlags,
                                           DNSServiceErrorType errorCode,
                                           const char* name,
                                           const char* regtype,
                                           const char* domain, void* context);

private:
    DNSServiceRef m_DnsServiceRef;
    QSocketNotifier* m_pSocket;
    ZeroconfRecord finalRecord;
};
