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

class CIpcTests : public ::testing::Test
{
public:
	CIpcTests();
	virtual ~CIpcTests();
	void handleClientConnected(const CEvent&, void* vclient);
	void raiseQuitEvent();

private:
	void timeoutThread(void*);

public:
	bool m_quitOnClientConnect;
	bool m_clientConnected;
	bool m_timeoutCheck;
	double m_timeout;

private:
	CThread* m_timeoutThread;
};

TEST_F(CIpcTests, connectToServer)
{
	m_quitOnClientConnect = true;

	CSocketMultiplexer multiplexer;
	CEventQueue events;

	CIpcServer server;
	server.listen();

	events.adoptHandler(
		CIpcServer::getClientConnectedEvent(), &server,
		new TMethodEventJob<CIpcTests>(
			this, &CIpcTests::handleClientConnected));

	CIpcClient client;
	client.connect();

	m_timeoutCheck = true;
	m_timeout = ARCH->time() + 5; // 5 sec timeout.
	events.loop();

	EXPECT_EQ(true, m_clientConnected);
}

CIpcTests::CIpcTests() :
m_timeoutThread(nullptr),
m_quitOnClientConnect(false),
m_clientConnected(false),
m_timeoutCheck(false),
m_timeout(0)
{
	m_timeoutThread = new CThread(
		new TMethodJob<CIpcTests>(
		this, &CIpcTests::timeoutThread, nullptr));
}

CIpcTests::~CIpcTests()
{
	delete m_timeoutThread;
}


void
CIpcTests::handleClientConnected(const CEvent&, void* vclient)
{
	m_clientConnected = true;

	if (m_quitOnClientConnect) {
		raiseQuitEvent();
	}
}

void
CIpcTests::raiseQuitEvent() 
{
	EVENTQUEUE->addEvent(CEvent(CEvent::kQuit, nullptr));
}

void
CIpcTests::timeoutThread(void*)
{
	while (true) {
		if (!m_timeoutCheck) {
			ARCH->sleep(1);
			continue;
		}

		if (ARCH->time() > m_timeout) {
			LOG((CLOG_ERR "timeout"));
			raiseQuitEvent();
			m_timeoutCheck = false;
		}
	}
}