/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
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

// TODO: fix, tests failing intermittently on mac.
#ifndef WINAPI_CARBON

#define TEST_ENV

#include "test/global/TestEventQueue.h"
#include "ipc/IpcServer.h"
#include "ipc/IpcClient.h"
#include "ipc/IpcServerProxy.h"
#include "ipc/IpcMessage.h"
#include "ipc/IpcClientProxy.h"
#include "ipc/Ipc.h"
#include "net/SocketMultiplexer.h"
#include "mt/Thread.h"
#include "arch/Arch.h"
#include "base/TMethodJob.h"
#include "base/String.h"
#include "base/Log.h"
#include "base/EventQueue.h"
#include "base/TMethodEventJob.h"

#include "test/global/gtest.h"

#define TEST_IPC_PORT 24802

class IpcTests : public ::testing::Test {
public:
    IpcTests ();
    virtual ~IpcTests ();

    void connectToServer_handleMessageReceived (const Event&, void*);
    void sendMessageToServer_serverHandleMessageReceived (const Event&, void*);
    void sendMessageToClient_serverHandleClientConnected (const Event&, void*);
    void sendMessageToClient_clientHandleMessageReceived (const Event&, void*);

public:
    SocketMultiplexer m_multiplexer;
    bool m_connectToServer_helloMessageReceived;
    bool m_connectToServer_hasClientNode;
    IpcServer* m_connectToServer_server;
    String m_sendMessageToServer_receivedString;
    String m_sendMessageToClient_receivedString;
    IpcClient* m_sendMessageToServer_client;
    IpcServer* m_sendMessageToClient_server;
    TestEventQueue m_events;
};

TEST_F (IpcTests, connectToServer) {
    SocketMultiplexer socketMultiplexer;
    IpcServer server (&m_events, &socketMultiplexer, TEST_IPC_PORT);
    server.listen ();
    m_connectToServer_server = &server;

    m_events.adoptHandler (
        m_events.forIpcServer ().messageReceived (),
        &server,
        new TMethodEventJob<IpcTests> (
            this, &IpcTests::connectToServer_handleMessageReceived));

    IpcClient client (&m_events, &socketMultiplexer, TEST_IPC_PORT);
    client.connect ();

    m_events.initQuitTimeout (5);
    m_events.loop ();
    m_events.removeHandler (m_events.forIpcServer ().messageReceived (),
                            &server);
    m_events.cleanupQuitTimeout ();

    EXPECT_EQ (true, m_connectToServer_helloMessageReceived);
    EXPECT_EQ (true, m_connectToServer_hasClientNode);
}

TEST_F (IpcTests, sendMessageToServer) {
    SocketMultiplexer socketMultiplexer;
    IpcServer server (&m_events, &socketMultiplexer, TEST_IPC_PORT);
    server.listen ();

    // event handler sends "test" command to server.
    m_events.adoptHandler (
        m_events.forIpcServer ().messageReceived (),
        &server,
        new TMethodEventJob<IpcTests> (
            this, &IpcTests::sendMessageToServer_serverHandleMessageReceived));

    IpcClient client (&m_events, &socketMultiplexer, TEST_IPC_PORT);
    client.connect ();
    m_sendMessageToServer_client = &client;

    m_events.initQuitTimeout (5);
    m_events.loop ();
    m_events.removeHandler (m_events.forIpcServer ().messageReceived (),
                            &server);
    m_events.cleanupQuitTimeout ();

    EXPECT_EQ ("test", m_sendMessageToServer_receivedString);
}

TEST_F (IpcTests, sendMessageToClient) {
    SocketMultiplexer socketMultiplexer;
    IpcServer server (&m_events, &socketMultiplexer, TEST_IPC_PORT);
    server.listen ();
    m_sendMessageToClient_server = &server;

    // event handler sends "test" log line to client.
    m_events.adoptHandler (
        m_events.forIpcServer ().messageReceived (),
        &server,
        new TMethodEventJob<IpcTests> (
            this, &IpcTests::sendMessageToClient_serverHandleClientConnected));

    IpcClient client (&m_events, &socketMultiplexer, TEST_IPC_PORT);
    client.connect ();

    m_events.adoptHandler (
        m_events.forIpcClient ().messageReceived (),
        &client,
        new TMethodEventJob<IpcTests> (
            this, &IpcTests::sendMessageToClient_clientHandleMessageReceived));

    m_events.initQuitTimeout (5);
    m_events.loop ();
    m_events.removeHandler (m_events.forIpcServer ().messageReceived (),
                            &server);
    m_events.removeHandler (m_events.forIpcClient ().messageReceived (),
                            &client);
    m_events.cleanupQuitTimeout ();

    EXPECT_EQ ("test", m_sendMessageToClient_receivedString);
}

IpcTests::IpcTests ()
    : m_connectToServer_helloMessageReceived (false),
      m_connectToServer_hasClientNode (false),
      m_connectToServer_server (nullptr),
      m_sendMessageToClient_server (nullptr),
      m_sendMessageToServer_client (nullptr) {
}

IpcTests::~IpcTests () {
}

void
IpcTests::connectToServer_handleMessageReceived (const Event& e, void*) {
    IpcMessage* m = static_cast<IpcMessage*> (e.getDataObject ());
    if (m->type () == kIpcHello) {
        m_connectToServer_hasClientNode =
            m_connectToServer_server->hasClients (kIpcClientNode);
        m_connectToServer_helloMessageReceived = true;
        m_events.raiseQuitEvent ();
    }
}

void
IpcTests::sendMessageToServer_serverHandleMessageReceived (const Event& e,
                                                           void*) {
    IpcMessage* m = static_cast<IpcMessage*> (e.getDataObject ());
    if (m->type () == kIpcHello) {
        LOG ((CLOG_DEBUG "client said hello, sending test to server"));
        IpcCommandMessage m ("test", true);
        m_sendMessageToServer_client->send (m);
    } else if (m->type () == kIpcCommand) {
        IpcCommandMessage* cm = static_cast<IpcCommandMessage*> (m);
        LOG ((CLOG_DEBUG "got ipc command message, %d",
              cm->command ().c_str ()));
        m_sendMessageToServer_receivedString = cm->command ();
        m_events.raiseQuitEvent ();
    }
}

void
IpcTests::sendMessageToClient_serverHandleClientConnected (const Event& e,
                                                           void*) {
    IpcMessage* m = static_cast<IpcMessage*> (e.getDataObject ());
    if (m->type () == kIpcHello) {
        LOG ((CLOG_DEBUG "client said hello, sending test to client"));
        IpcLogLineMessage m ("test");
        m_sendMessageToClient_server->send (m, kIpcClientNode);
    }
}

void
IpcTests::sendMessageToClient_clientHandleMessageReceived (const Event& e,
                                                           void*) {
    IpcMessage* m = static_cast<IpcMessage*> (e.getDataObject ());
    if (m->type () == kIpcLogLine) {
        IpcLogLineMessage* llm = static_cast<IpcLogLineMessage*> (m);
        LOG ((CLOG_DEBUG "got ipc log message, %d", llm->logLine ().c_str ()));
        m_sendMessageToClient_receivedString = llm->logLine ();
        m_events.raiseQuitEvent ();
    }
}

#endif // WINAPI_CARBON
