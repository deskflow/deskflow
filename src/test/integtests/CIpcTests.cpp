/*
 * synergy -- mouse and keyboard sharing utility
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
#include "CSimpleEventQueueBuffer.h"

class CIpcTests : public ::testing::Test
{
public:
	CIpcTests();
	virtual ~CIpcTests();
	
	void				connectToServer_handleMessageReceived(const CEvent&, void*);
	void				sendMessageToServer_handleClientConnected(const CEvent&, void*);
	void				sendMessageToServer_handleMessageReceived(const CEvent&, void*);
	void				sendMessageToClient_handleClientConnected(const CEvent&, void*);
	void				sendMessageToClient_handleMessageReceived(const CEvent&, void*);
	void				handleQuitTimeout(const CEvent&, void* vclient);
	void				raiseQuitEvent();
	void				initQuitTimeout(double timeout);
	void				cleanupQuitTimeout();

private:
	void				timeoutThread(void*);

public:
	CSocketMultiplexer	m_multiplexer;
	CEventQueue			m_events;
	CEventQueueTimer*	m_quitTimeoutTimer;
	bool				m_connectToServer_helloMessageReceived;
	bool				m_connectToServer_hasClientNode;
	CIpcServer*			m_connectToServer_server;
	CString				m_sendMessageToServer_receivedString;
	CString				m_sendMessageToClient_receivedString;
	CIpcClient*			m_sendMessageToServer_client;
	CIpcServer*			m_sendMessageToClient_server;

};

TEST_F(CIpcTests, connectToServer)
{
	CIpcServer server;
	server.listen();
	m_connectToServer_server = &server;

	m_events.adoptHandler(
		CIpcServer::getMessageReceivedEvent(), &server,
		new TMethodEventJob<CIpcTests>(
		this, &CIpcTests::connectToServer_handleMessageReceived));

	CIpcClient client;
	client.connect();
	
	initQuitTimeout(2);
	m_events.loop();
	m_events.removeHandler(CIpcServer::getMessageReceivedEvent(), &server);
	cleanupQuitTimeout();
	
	EXPECT_EQ(true, m_connectToServer_helloMessageReceived);
	EXPECT_EQ(true, m_connectToServer_hasClientNode);
}

TEST_F(CIpcTests, sendMessageToServer)
{
	CIpcServer server;
	server.listen();
	
	// event handler sends "test" command to server.
	m_events.adoptHandler(
		CIpcServer::getClientConnectedEvent(), &server,
		new TMethodEventJob<CIpcTests>(
		this, &CIpcTests::sendMessageToServer_handleClientConnected));

	m_events.adoptHandler(
		CIpcServer::getMessageReceivedEvent(), &server,
		new TMethodEventJob<CIpcTests>(
		this, &CIpcTests::sendMessageToServer_handleMessageReceived));

	CIpcClient client;
	client.connect();
	m_sendMessageToServer_client = &client;

	initQuitTimeout(2);
	m_events.loop();
	m_events.removeHandler(CIpcServer::getClientConnectedEvent(), &server);
	m_events.removeHandler(CIpcServer::getMessageReceivedEvent(), &server);
	cleanupQuitTimeout();

	EXPECT_EQ("test", m_sendMessageToServer_receivedString);
}

TEST_F(CIpcTests, sendMessageToClient)
{
	CIpcServer server;
	server.listen();
	m_sendMessageToClient_server = &server;

	// event handler sends "test" log line to client.
	m_events.adoptHandler(
		CIpcServer::getClientConnectedEvent(), &server,
		new TMethodEventJob<CIpcTests>(
		this, &CIpcTests::sendMessageToClient_handleClientConnected));

	CIpcClient client;
	client.connect();
	
	m_events.adoptHandler(
		CIpcClient::getMessageReceivedEvent(), &client,
		new TMethodEventJob<CIpcTests>(
		this, &CIpcTests::sendMessageToClient_handleMessageReceived));

	initQuitTimeout(2);
	m_events.loop();
	m_events.removeHandler(CIpcServer::getClientConnectedEvent(), &server);
	m_events.removeHandler(CIpcClient::getMessageReceivedEvent(), &client);
	cleanupQuitTimeout();

	EXPECT_EQ("test", m_sendMessageToClient_receivedString);
}

CIpcTests::CIpcTests() :
m_quitTimeoutTimer(nullptr),
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
		raiseQuitEvent();
	}
}

void
CIpcTests::sendMessageToServer_handleClientConnected(const CEvent& e, void*)
{	
	CIpcCommandMessage m("test");
	m_sendMessageToServer_client->send(m);
}

void
CIpcTests::sendMessageToServer_handleMessageReceived(const CEvent& e, void*)
{
	CIpcMessage* m = static_cast<CIpcMessage*>(e.getDataObject());
	if (m->m_type == kIpcCommand) {
		CIpcCommandMessage* cm = static_cast<CIpcCommandMessage*>(m);
		m_sendMessageToServer_receivedString = cm->command();
		raiseQuitEvent();
	}
}

void
CIpcTests::sendMessageToClient_handleClientConnected(const CEvent& e, void*)
{	
	CIpcLogLineMessage m("test");
	m_sendMessageToClient_server->send(m, kIpcClientUnknown);
}

void
CIpcTests::sendMessageToClient_handleMessageReceived(const CEvent& e, void*)
{
	CIpcMessage* m = static_cast<CIpcMessage*>(e.getDataObject());
	if (m->m_type == kIpcLogLine) {
		CIpcLogLineMessage* llm = static_cast<CIpcLogLineMessage*>(m);
		m_sendMessageToClient_receivedString = llm->logLine();
		raiseQuitEvent();
	}
}

void
CIpcTests::raiseQuitEvent() 
{
	EVENTQUEUE->addEvent(CEvent(CEvent::kQuit));
}

void
CIpcTests::initQuitTimeout(double timeout)
{
	assert(m_quitTimeoutTimer == nullptr);
	m_quitTimeoutTimer = EVENTQUEUE->newOneShotTimer(timeout, NULL);
	EVENTQUEUE->adoptHandler(CEvent::kTimer, m_quitTimeoutTimer,
		new TMethodEventJob<CIpcTests>(
		this, &CIpcTests::handleQuitTimeout));
}

void
CIpcTests::cleanupQuitTimeout()
{
	EVENTQUEUE->removeHandler(CEvent::kTimer, m_quitTimeoutTimer);
	delete m_quitTimeoutTimer;
	m_quitTimeoutTimer = nullptr;
}

void
CIpcTests::handleQuitTimeout(const CEvent&, void* vclient)
{
	LOG((CLOG_ERR "timeout"));
	raiseQuitEvent();
}
