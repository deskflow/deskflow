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

class CIpcTests : public ::testing::Test
{
public:
	CIpcTests();
	virtual ~CIpcTests();
	
	void				connectToServer_handleClientConnected(const CEvent&, void* vclient);
	void				sendMessageToServer_handleClientConnected(const CEvent&, void* vclient);
	void				sendMessageToServer_handleMessageReceived(const CEvent&, void* vclient);
	void				handleQuitTimeout(const CEvent&, void* vclient);
	void				raiseQuitEvent();
	void				quitTimeout(double timeout);

private:
	void				timeoutThread(void*);

public:
	CSocketMultiplexer	m_multiplexer;
	CEventQueue			m_events;
	bool				m_connectToServer_clientConnected;
	CString				m_sendMessageToServer_receivedString;
};

TEST_F(CIpcTests, connectToServer)
{
	CIpcServer server;
	server.listen();

	m_events.adoptHandler(
		CIpcServer::getClientConnectedEvent(), &server,
		new TMethodEventJob<CIpcTests>(
		this, &CIpcTests::connectToServer_handleClientConnected));

	CIpcClient client;
	client.connect();
	
	quitTimeout(2);
	m_events.loop();

	EXPECT_EQ(true, m_connectToServer_clientConnected);
}

TEST_F(CIpcTests, sendMessageToServer)
{
	CIpcServer server;
	server.listen();

	CIpcClient client;
	client.connect();
	
	m_events.adoptHandler(
		CIpcServer::getClientConnectedEvent(), &server,
		new TMethodEventJob<CIpcTests>(
		this, &CIpcTests::sendMessageToServer_handleClientConnected));
	
	CIpcMessage m;
	m.m_type = kIpcCommand;
	m.m_data = (void*)(new CString("test"));
	client.send(m);

	quitTimeout(2);
	m_events.loop();

	EXPECT_EQ("test", m_sendMessageToServer_receivedString);
}

CIpcTests::CIpcTests() :
m_connectToServer_clientConnected(false)
{
}

CIpcTests::~CIpcTests()
{
}

void
CIpcTests::connectToServer_handleClientConnected(const CEvent&, void*)
{
	m_connectToServer_clientConnected = true;
	raiseQuitEvent();
}

void
CIpcTests::sendMessageToServer_handleClientConnected(const CEvent& e, void*)
{
	m_events.adoptHandler(
		CIpcClientProxy::getMessageReceivedEvent(), e.getData(),
		new TMethodEventJob<CIpcTests>(
		this, &CIpcTests::sendMessageToServer_handleMessageReceived));
}

void
CIpcTests::sendMessageToServer_handleMessageReceived(const CEvent& e, void*)
{
	CIpcMessage* m = (CIpcMessage*)e.getData();
	m_sendMessageToServer_receivedString = *((CString*)m->m_data);
	raiseQuitEvent();
}

void
CIpcTests::raiseQuitEvent() 
{
	EVENTQUEUE->addEvent(CEvent(CEvent::kQuit, nullptr));
}

void
CIpcTests::quitTimeout(double timeout)
{
	CEventQueueTimer* timer = EVENTQUEUE->newOneShotTimer(timeout, NULL);
	EVENTQUEUE->adoptHandler(CEvent::kTimer, timer,
		new TMethodEventJob<CIpcTests>(this, &CIpcTests::handleQuitTimeout, timer));
}

void
CIpcTests::handleQuitTimeout(const CEvent&, void* vclient)
{
	LOG((CLOG_ERR "timeout"));
	raiseQuitEvent();
}
