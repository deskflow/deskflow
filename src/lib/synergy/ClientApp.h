/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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

#include "synergy/App.h"

namespace synergy {
class Screen;
}
class Event;
class Client;
class NetworkAddress;
class Thread;
class ClientArgs;

class ClientApp : public App {
public:
    ClientApp (IEventQueue* events,
               CreateTaskBarReceiverFunc createTaskBarReceiver);
    virtual ~ClientApp ();

    // Parse client specific command line arguments.
    void parseArgs (int argc, const char* const* argv);

    // Prints help specific to client.
    void help ();

    // Returns arguments that are common and for client.
    ClientArgs&
    args () const {
        return (ClientArgs&) argsBase ();
    }

    const char* daemonName () const;
    const char* daemonInfo () const;

    // TODO: move to server only (not supported on client)
    void
    loadConfig () {
    }
    bool
    loadConfig (const String& pathname) {
        return false;
    }

    int foregroundStartup (int argc, char** argv);
    int standardStartup (int argc, char** argv);
    int runInner (int argc, char** argv, ILogOutputter* outputter,
                  StartupFunc startup);
    synergy::Screen* createScreen ();
    void updateStatus ();
    void updateStatus (const String& msg);
    void resetRestartTimeout ();
    double nextRestartTimeout ();
    void handleScreenError (const Event&, void*);
    synergy::Screen* openClientScreen ();
    void closeClientScreen (synergy::Screen* screen);
    void handleClientRestart (const Event&, void* vtimer);
    void scheduleClientRestart (double retryTime);
    void handleClientConnected (const Event&, void*);
    void handleClientFailed (const Event& e, void*);
    void handleClientDisconnected (const Event&, void*);
    Client* openClient (const String& name, const NetworkAddress& address,
                        synergy::Screen* screen);
    void closeClient (Client* client);
    bool startClient ();
    void stopClient ();
    int mainLoop ();
    void startNode ();

    static ClientApp&
    instance () {
        return (ClientApp&) App::instance ();
    }

    Client*
    getClientPtr () {
        return m_client;
    }

private:
    Client* m_client;
    synergy::Screen* m_clientScreen;
    NetworkAddress* m_serverAddress;
};
