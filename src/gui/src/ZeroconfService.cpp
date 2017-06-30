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

#include "ZeroconfService.h"

#include "MainWindow.h"
#include "ZeroconfRegister.h"
#include "ZeroconfBrowser.h"

#include <QtNetwork>
#include <QMessageBox>
#define _MSL_STDINT_H
#include <stdint.h>
#include <dns_sd.h>

static const QStringList preferedIPAddress (QStringList () << "192.168."
                                                           << "10."
                                                           << "172.");

const char* ZeroconfService::m_ServerServiceName =
    "_synergyServerZeroconf._tcp";
const char* ZeroconfService::m_ClientServiceName =
    "_synergyClientZeroconf._tcp";

ZeroconfService::ZeroconfService (MainWindow* mainWindow)
    : m_pMainWindow (mainWindow),
      m_pZeroconfBrowser (0),
      m_pZeroconfRegister (0),
      m_ServiceRegistered (false) {
    if (m_pMainWindow->synergyType () == MainWindow::synergyServer) {
        if (registerService (true)) {
            m_pZeroconfBrowser = new ZeroconfBrowser (this);
            connect (
                m_pZeroconfBrowser,
                SIGNAL (currentRecordsChanged (const QList<ZeroconfRecord>&)),
                this,
                SLOT (clientDetected (const QList<ZeroconfRecord>&)));
            m_pZeroconfBrowser->browseForType (
                QLatin1String (m_ClientServiceName));
        }
    } else {
        m_pZeroconfBrowser = new ZeroconfBrowser (this);
        connect (m_pZeroconfBrowser,
                 SIGNAL (currentRecordsChanged (const QList<ZeroconfRecord>&)),
                 this,
                 SLOT (serverDetected (const QList<ZeroconfRecord>&)));
        m_pZeroconfBrowser->browseForType (QLatin1String (m_ServerServiceName));
    }

    connect (m_pZeroconfBrowser,
             SIGNAL (error (DNSServiceErrorType)),
             this,
             SLOT (errorHandle (DNSServiceErrorType)));
}

ZeroconfService::~ZeroconfService () {
    if (m_pZeroconfBrowser) {
        delete m_pZeroconfBrowser;
    }
    if (m_pZeroconfRegister) {
        delete m_pZeroconfRegister;
    }
}

void
ZeroconfService::serverDetected (const QList<ZeroconfRecord>& list) {
    foreach (ZeroconfRecord record, list) {
        registerService (false);
        m_pMainWindow->appendLogInfo (
            tr ("zeroconf server detected: %1").arg (record.serviceName));
        m_pMainWindow->serverDetected (record.serviceName);
    }
}

void
ZeroconfService::clientDetected (const QList<ZeroconfRecord>& list) {
    foreach (ZeroconfRecord record, list) {
        m_pMainWindow->appendLogInfo (
            tr ("zeroconf client detected: %1").arg (record.serviceName));
        m_pMainWindow->autoAddScreen (record.serviceName);
    }
}

void
ZeroconfService::errorHandle (DNSServiceErrorType errorCode) {
    QMessageBox::critical (0,
                           tr ("Zero configuration service"),
                           tr ("Error code: %1.").arg (errorCode));
}

QString
ZeroconfService::getLocalIPAddresses () {
    QStringList addresses;
    foreach (const QHostAddress& address, QNetworkInterface::allAddresses ()) {
        if (address.protocol () == QAbstractSocket::IPv4Protocol &&
            address != QHostAddress (QHostAddress::LocalHost)) {
            addresses.append (address.toString ());
        }
    }

    foreach (const QString& preferedIP, preferedIPAddress) {
        foreach (const QString& address, addresses) {
            if (address.startsWith (preferedIP)) {
                return address;
            }
        }
    }

    return "";
}

bool
ZeroconfService::registerService (bool server) {
    bool result = true;

    if (!m_ServiceRegistered) {
        if (!m_zeroconfServer.listen ()) {
            QMessageBox::critical (0,
                                   tr ("Zero configuration service"),
                                   tr ("Unable to start the zeroconf: %1.")
                                       .arg (m_zeroconfServer.errorString ()));
            result = false;
        } else {
            m_pZeroconfRegister = new ZeroconfRegister (this);
            if (server) {
                QString localIP = getLocalIPAddresses ();
                if (localIP.isEmpty ()) {
                    QMessageBox::warning (
                        m_pMainWindow,
                        tr ("Synergy"),
                        tr ("Failed to get local IP address. "
                            "Please manually type in server address "
                            "on your clients"));
                } else {
                    m_pZeroconfRegister->registerService (
                        ZeroconfRecord (tr ("%1").arg (localIP),
                                        QLatin1String (m_ServerServiceName),
                                        QString ()),
                        m_zeroconfServer.serverPort ());
                }
            } else {
                m_pZeroconfRegister->registerService (
                    ZeroconfRecord (
                        tr ("%1").arg (m_pMainWindow->getScreenName ()),
                        QLatin1String (m_ClientServiceName),
                        QString ()),
                    m_zeroconfServer.serverPort ());
            }

            m_ServiceRegistered = true;
        }
    }

    return result;
}
