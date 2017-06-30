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

#pragma once

#include "server/Server.h"
#include "synergy/ServerApp.h"
#include "arch/IArchTaskBarReceiver.h"
#include "base/EventTypes.h"
#include "base/String.h"
#include "base/Event.h"
#include "common/stdvector.h"

class IEventQueue;

//! Implementation of IArchTaskBarReceiver for the synergy server
class ServerTaskBarReceiver : public IArchTaskBarReceiver {
public:
    ServerTaskBarReceiver (IEventQueue* events);
    virtual ~ServerTaskBarReceiver ();

    //! @name manipulators
    //@{

    //! Update status
    /*!
    Determine the status and query required information from the server.
    */
    void updateStatus (Server*, const String& errorMsg);

    void
    updateStatus (INode* n, const String& errorMsg) {
        updateStatus ((Server*) n, errorMsg);
    }

    //@}

    // IArchTaskBarReceiver overrides
    virtual void showStatus () = 0;
    virtual void runMenu (int x, int y) = 0;
    virtual void primaryAction () = 0;
    virtual void lock () const;
    virtual void unlock () const;
    virtual const Icon getIcon () const = 0;
    virtual std::string getToolTip () const;

protected:
    typedef std::vector<String> Clients;
    enum EState {
        kNotRunning,
        kNotWorking,
        kNotConnected,
        kConnected,
        kMaxState
    };

    //! Get status
    EState getStatus () const;

    //! Get error message
    const String& getErrorMessage () const;

    //! Get connected clients
    const Clients& getClients () const;

    //! Quit app
    /*!
    Causes the application to quit gracefully
    */
    void quit ();

    //! Status change notification
    /*!
    Called when status changes.  The default implementation does
    nothing.
    */
    virtual void onStatusChanged (Server* server);

private:
    EState m_state;
    String m_errorMessage;
    Clients m_clients;
    IEventQueue* m_events;
};

IArchTaskBarReceiver*
createTaskBarReceiver (const BufferedLogOutputter* logBuffer,
                       IEventQueue* events);
