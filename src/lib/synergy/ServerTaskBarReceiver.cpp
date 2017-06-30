/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2003 Chris Schoeneman
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

#include "synergy/ServerTaskBarReceiver.h"
#include "server/Server.h"
#include "mt/Lock.h"
#include "base/String.h"
#include "base/IEventQueue.h"
#include "arch/Arch.h"
#include "common/Version.h"

//
// ServerTaskBarReceiver
//

ServerTaskBarReceiver::ServerTaskBarReceiver (IEventQueue* events)
    : m_state (kNotRunning), m_events (events) {
    // do nothing
}

ServerTaskBarReceiver::~ServerTaskBarReceiver () {
    // do nothing
}

void
ServerTaskBarReceiver::updateStatus (Server* server, const String& errorMsg) {
    {
        // update our status
        m_errorMessage = errorMsg;
        if (server == NULL) {
            if (m_errorMessage.empty ()) {
                m_state = kNotRunning;
            } else {
                m_state = kNotWorking;
            }
        } else {
            m_clients.clear ();
            server->getClients (m_clients);
            if (m_clients.size () <= 1) {
                m_state = kNotConnected;
            } else {
                m_state = kConnected;
            }
        }

        // let subclasses have a go
        onStatusChanged (server);
    }

    // tell task bar
    ARCH->updateReceiver (this);
}

ServerTaskBarReceiver::EState
ServerTaskBarReceiver::getStatus () const {
    return m_state;
}

const String&
ServerTaskBarReceiver::getErrorMessage () const {
    return m_errorMessage;
}

const ServerTaskBarReceiver::Clients&
ServerTaskBarReceiver::getClients () const {
    return m_clients;
}

void
ServerTaskBarReceiver::quit () {
    m_events->addEvent (Event (Event::kQuit));
}

void
ServerTaskBarReceiver::onStatusChanged (Server*) {
    // do nothing
}

void
ServerTaskBarReceiver::lock () const {
    // do nothing
}

void
ServerTaskBarReceiver::unlock () const {
    // do nothing
}

std::string
ServerTaskBarReceiver::getToolTip () const {
    switch (m_state) {
        case kNotRunning:
            return synergy::string::sprintf ("%s:  Not running", kAppVersion);

        case kNotWorking:
            return synergy::string::sprintf (
                "%s:  %s", kAppVersion, m_errorMessage.c_str ());

        case kNotConnected:
            return synergy::string::sprintf ("%s:  Waiting for clients",
                                             kAppVersion);

        case kConnected:
            return synergy::string::sprintf ("%s:  Connected", kAppVersion);

        default:
            return "";
    }
}
