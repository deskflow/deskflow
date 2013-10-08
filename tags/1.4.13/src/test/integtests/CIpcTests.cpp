/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
 * Copyright (C) 2012 Nick Bolton
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
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

#include <gtest/gtest.h>

#define TEST_ENV
#include "Global.h"

#include "CIpcServer.h"
#include "CIpcClient.h"
#include "CSocketMultiplexer.h"
#include "CEventQueue.h"
#include "TMethodEventJob.h"
#include "CThread.h"
#include "TMethodJob.h"
#include "CArch.h"
#include "CLog.h"
#include "CIpcClientProxy.h"
#include "Ipc.h"
#include "CString.h"
#include "CIpcServerProxy.h"
#include "CIpcMessage.h"
#include "CTestEventQueue.h"

#define TEST_IPC_PORT 24802

class CIpcTests : public ::testing::Test
{
public:
	CIpcTests();
	virtual ~CIpcTests();
	
	void				connectToServer_handleMessageReceived(const CEvent&, void*);
	void				sendMessageToServer_serverHandleMessageReceived(const CEvent&, void*);
	void				sendMessageToClient_serverHandleClientConnected(const CEvent&, void*);
	void				sendMessageToClient_clientHandleMessageReceived(const CEvent&, void*);

public:
	CSocketMultiplexer	m_multiplexer;
	bool				m_connectToServer_helloMessageReceived;
	bool				m_connectToServer_hasClientNode;
	CIpcServer*			m_connectToServer_server;
	CString				m_sendMessageToServer_receivedString;
	CString				m_sendMessageToClient_receivedString;
	CIpcClient*			m_sendMessageToServer_client;
	CIpcServer*			m_sendMessageToClient_server;
	CTestEventQueue		m_events;

};

TEST_F(CIpcTests, connectToServer)
{
	CSocketMultiplexer socketMultiplexer;
	CIpcServer server(&m_events, &socketMultiplexer, TEST_IPC_PORT);
	server.listen();
	m_connectToServer_server = &server;

	m_events.adoptHandler(
		m_events.forCIpcServer().messageReceived(), &server,
		new TMethodEventJob<CIpcTests>(
		this, &CIpcTests::connectToServer_handleMessageReceived));
	
	CIpcClient client(&m_events, &socketMultiplexer, TEST_IPC_PORT);
	client.connect();
	
	m_events.initQuitTimeout(5);
	m_events.loop();
	m_events.removeHandler(m_events.forCIpcServer().messageReceived(), &server);
	m_events.cleanupQuitTimeout();
	
	EXPECT_EQ(true, m_connectToServer_helloMessageReceived);
	EXPECT_EQ(true, m_connectToServer_hasClientNode);
}

TEST_F(CIpcTests, sendMessageToServer)
{
	CSocketMultiplexer socketMultiplexer;
	CIpcServer server(&m_events, &socketMultiplexer, TEST_IPC_PORT);
	server.listen();
	
	// event handler sends "test" command to server.
	m_events.adoptHandler(
		m_events.forCIpcServer().messageReceived(), &server,
		new TMethodEventJob<CIpcTests>(
		this, &CIpcTests::sendMessageToServer_serverHandleMessageReceived));
	
	CIpcClient client(&m_events, &socketMultiplexer, TEST_IPC_PORT);
	client.connect();
	m_sendMessageToServer_client = &client;

	m_events.initQuitTimeout(5);
	m_events.loop();
	m_events.removeHandler(m_events.forCIpcServer().messageReceived(), &server);
	m_events.cleanupQuitTimeout();

	EXPECT_EQ("test", m_sendMessageToServer_receivedString);
}

TEST_F(CIpcTests, sendMessageToClient)
{
	CSocketMultiplexer socketMultiplexer;
	CIpcServer server(&m_events, &socketMultiplexer, TEST_IPC_PORT);
	server.listen();
	m_sendMessageToClient_server = &server;

	// event handler sends "test" log line to client.
	m_events.adoptHandler(
		m_events.forCIpcServer().messageReceived(), &server,
		new TMethodEventJob<CIpcTests>(
		this, &CIpcTests::sendMessageToClient_serverHandleClientConnected));

	CIpcClient client(&m_events, &socketMultiplexer, TEST_IPC_PORT);
	client.connect();
	
	m_events.adoptHandler(
		m_events.forCIpcClient().messageReceived(), &client,
		new TMethodEventJob<CIpcTests>(
		this, &CIpcTests::sendMessageToClient_clientHandleMessageReceived));

	m_events.initQuitTimeout(5);
	m_events.loop();
	m_events.removeHandler(m_events.forCIpcServer().messageReceived(), &server);
	m_events.removeHandler(m_events.forCIpcClient().messageReceived(), &client);
	m_events.cleanupQuitTimeout();

	EXPECT_EQ("test", m_sendMessageToClient_receivedString);
}

CIpcTests::CIpcTests() :
m_connectToServer_helloMessageReceived(false),
m_connectToServer_hasClientNode(false),
m_connectToServer_server(nullptr),
m_sendMessageToClient_server(nullptr),
m_sendMessageToServer_client(nullptr)
{
}

CIpcTests::~CIpcTests()
{
}

void
CIpcTests::connectToServer_handleMessageReceived(const CEvent& e, void*)
{
	CIpcMessage* m = static_cast<CIpcMessage*>(e.getDataObject());
	if (m->m_type == kIpcHello) {
		m_connectToServer_hasClientNode =
			m_connectToServer_server->hasClients(kIpcClientNode);
		m_connectToServer_helloMessageReceived = true;
		m_events.raiseQuitEvent();
	}
}

void
CIpcTests::sendMessageToServer_serverHandleMessageReceived(const CEvent& e, void*)
{
	CIpcMessage* m = static_cast<CIpcMessage*>(e.getDataObject());
	if (m->m_type == kIpcHello) {
		LOG((CLOG_DEBUG "client said hello, sending test to server"));
		CIpcCommandMessage m("test", true);
		m_sendMessageToServer_client->send(m);
	}
	else if (m->m_type == kIpcCommand) {
		CIpcCommandMessage* cm = static_cast<CIpcCommandMessage*>(m);
		LOG((CLOG_DEBUG "got ipc command message, %d", cm->command().c_str()));
		m_sendMessageToServer_receivedString = cm->command();
		m_events.raiseQuitEvent();
	}
}

void
CIpcTests::sendMessageToClient_serverHandleClientConnected(const CEvent& e, void*)
{
	CIpcMessage* m = static_cast<CIpcMessage*>(e.getDataObject());
	if (m->m_type == kIpcHello) {
		LOG((CLOG_DEBUG "client said hello, sending test to client"));
		CIpcLogLineMessage m("test");
		m_sendMessageToClient_server->send(m, kIpcClientNode);
	}
}

void
CIpcTests::sendMessageToClient_clientHandleMessageReceived(const CEvent& e, void*)
{
	CIpcMessage* m = static_cast<CIpcMessage*>(e.getDataObject());
	if (m->m_type == kIpcLogLine) {
		CIpcLogLineMessage* llm = static_cast<CIpcLogLineMessage*>(m);
		LOG((CLOG_DEBUG "got ipc log message, %d", llm->logLine().c_str()));
		m_sendMessageToClient_receivedString = llm->logLine();
		m_events.raiseQuitEvent();
	}
}

#endif // WINAPI_CARBON
