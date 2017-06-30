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

#include "ZeroconfBrowser.h"

#include <QtCore/QSocketNotifier>

ZeroconfBrowser::ZeroconfBrowser (QObject* parent)
    : QObject (parent), m_DnsServiceRef (0), m_pSocket (0) {
}

ZeroconfBrowser::~ZeroconfBrowser () {
    if (m_pSocket) {
        delete m_pSocket;
    }

    if (m_DnsServiceRef) {
        DNSServiceRefDeallocate (m_DnsServiceRef);
        m_DnsServiceRef = 0;
    }
}

void
ZeroconfBrowser::browseForType (const QString& type) {
    DNSServiceErrorType err = DNSServiceBrowse (&m_DnsServiceRef,
                                                0,
                                                0,
                                                type.toUtf8 ().constData (),
                                                0,
                                                browseReply,
                                                this);

    if (err != kDNSServiceErr_NoError) {
        emit error (err);
    } else {
        int sockFD = DNSServiceRefSockFD (m_DnsServiceRef);
        if (sockFD == -1) {
            emit error (kDNSServiceErr_Invalid);
        } else {
            m_pSocket =
                new QSocketNotifier (sockFD, QSocketNotifier::Read, this);
            connect (m_pSocket,
                     SIGNAL (activated (int)),
                     this,
                     SLOT (socketReadyRead ()));
        }
    }
}

void
ZeroconfBrowser::socketReadyRead () {
    DNSServiceErrorType err = DNSServiceProcessResult (m_DnsServiceRef);
    if (err != kDNSServiceErr_NoError) {
        emit error (err);
    }
}

void
ZeroconfBrowser::browseReply (DNSServiceRef, DNSServiceFlags flags, quint32,
                              DNSServiceErrorType errorCode,
                              const char* serviceName, const char* regType,
                              const char* replyDomain, void* context) {
    ZeroconfBrowser* browser = static_cast<ZeroconfBrowser*> (context);
    if (errorCode != kDNSServiceErr_NoError) {
        emit browser->error (errorCode);
    } else {
        ZeroconfRecord record (serviceName, regType, replyDomain);
        if (flags & kDNSServiceFlagsAdd) {
            if (!browser->m_Records.contains (record)) {
                browser->m_Records.append (record);
            }
        } else {
            browser->m_Records.removeAll (record);
        }
        if (!(flags & kDNSServiceFlagsMoreComing)) {
            emit browser->currentRecordsChanged (browser->m_Records);
        }
    }
}
