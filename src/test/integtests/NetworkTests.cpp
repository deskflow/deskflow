/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2013 Bolton Software Ltd.
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
#include <iostream>
#include <fstream>

#define TEST_ENV

#include "CLog.h"
#include "CServer.h"
#include "CClient.h"
#include "TMethodEventJob.h"
#include "server/CMockConfig.h"
#include "server/CMockPrimaryClient.h"
#include "synergy/CMockScreen.h"
#include "CClientListener.h"
#include "CNetworkAddress.h"
#include "CTCPSocketFactory.h"
#include "CCryptoOptions.h"
#include "CSocketMultiplexer.h"
#include "CMSWindowsScreen.h"
#include "CGameDevice.h"
#include "CThread.h"
#include "TMethodJob.h"
#include "CTestEventQueue.h"
#include "server/CMockInputFilter.h"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::Invoke;

#define TEST_PORT 24803
#define TEST_HOST "localhost"

const int klargeDataSize = 512;
char g_largeData[klargeDataSize] = "large data:head.1221412312341244213123fdsfasdawdwadwadacwdd.12321412312341244213123fdsfasdawdwadwadacwdawddawdwacawdawd232141231awddawdwacawdawd2321412312341244213123fdsfasdawdwadacwdawddawdwacawdtrtetawdawdwaewe1213412321412312341244213123fdsfasdawdwadacwdawddawdwacawdawdawdwaewe121341awdwaewedacwdawddawdwacawdawd2321412312341244213123fdsfasdawdwadacwdawddawdwacawdtrtetawdawdwaewe1213412321412312341244213123fdsfasdawdwadacwdawddawdwacawdawdawdwaewe121341awdwaewe12134123njk1u31i2nm3e123hu23oi132213njk.tail";

void sendFileToClient_getShape(SInt32& x, SInt32& y, SInt32& w, SInt32& h);
void sendFileToClient_getCursorPos(SInt32& x, SInt32& y);

class NetworkTests : public ::testing::Test
{
public:
	NetworkTests() { }

	void				sendData(CServer* server);
	
	void				sendFileToClient_handleClientConnected(const CEvent&, void* vlistener);
	void				sendFileToClient_fileRecieveComplete(const CEvent&, void*);

public:
	CTestEventQueue		m_events;
};

TEST_F(NetworkTests, sendFileToClient)
{
	// server and client
	CNetworkAddress serverAddress(TEST_HOST, TEST_PORT);
	CCryptoOptions cryptoOptions;
	
	serverAddress.resolve();
	
	// server
	CSocketMultiplexer serverSocketMultiplexer;
	CTCPSocketFactory* serverSocketFactory = new CTCPSocketFactory(&m_events, &serverSocketMultiplexer);
	CClientListener listener(serverAddress, serverSocketFactory, NULL, cryptoOptions, &m_events);
	NiceMock<CMockScreen> serverScreen;
	NiceMock<CMockPrimaryClient> primaryClient;
	NiceMock<CMockConfig> serverConfig;
	NiceMock<CMockInputFilter> serverInputFilter;
	
	m_events.adoptHandler(
		m_events.forCClientListener().connected(), &listener,
		new TMethodEventJob<NetworkTests>(
			this, &NetworkTests::sendFileToClient_handleClientConnected, &listener));

	ON_CALL(serverConfig, isScreen(_)).WillByDefault(Return(true));
	ON_CALL(serverConfig, getInputFilter()).WillByDefault(Return(&serverInputFilter));
	
	CServer server(serverConfig, &primaryClient, &serverScreen, &m_events);
	server.m_mock = true;
	listener.setServer(&server);

	// client
	NiceMock<CMockScreen> clientScreen;
	CSocketMultiplexer clientSocketMultiplexer;
	CTCPSocketFactory* clientSocketFactory = new CTCPSocketFactory(&m_events, &clientSocketMultiplexer);
	
	ON_CALL(clientScreen, getShape(_, _, _, _)).WillByDefault(Invoke(sendFileToClient_getShape));
	ON_CALL(clientScreen, getCursorPos(_, _)).WillByDefault(Invoke(sendFileToClient_getCursorPos));

	CClient client(&m_events, "stub", serverAddress, clientSocketFactory, NULL, &clientScreen, cryptoOptions);
		
	m_events.adoptHandler(
		m_events.forIScreen().fileRecieveComplete(), &client,
		new TMethodEventJob<NetworkTests>(
			this, &NetworkTests::sendFileToClient_fileRecieveComplete));

	client.connect();

	m_events.initQuitTimeout(10);
	m_events.loop();
	m_events.cleanupQuitTimeout();
}

void 
NetworkTests::sendFileToClient_handleClientConnected(const CEvent&, void* vlistener)
{
	CClientListener* listener = reinterpret_cast<CClientListener*>(vlistener);
	CServer* server = listener->getServer();

	CClientProxy* client = listener->getNextClient();
	if (client == NULL) {
		throw std::exception("client is null");
	}

	CBaseClientProxy* bcp = reinterpret_cast<CBaseClientProxy*>(client);
	server->adoptClient(bcp);
	server->setActive(bcp);

	sendData(server);
}

void 
NetworkTests::sendFileToClient_fileRecieveComplete(const CEvent& event, void*)
{
	CClient* client = reinterpret_cast<CClient*>(event.getTarget());
	EXPECT_TRUE(client->isReceivedFileSizeValid());

	m_events.raiseQuitEvent();
}

void 
NetworkTests::sendData(CServer* server)
{
	UInt8* largeDataSize = new UInt8[5];
	largeDataSize[0] = '0';
	largeDataSize[1] = '5';
	largeDataSize[2] = '1';
	largeDataSize[3] = '1';
	largeDataSize[4] = '\0';
	
	// transfer data from server -> client
	m_events.addEvent(CEvent(m_events.forIScreen().fileChunkSending(), server, largeDataSize));
	
	UInt8* largeData = new UInt8[klargeDataSize + 1];
	largeData[0] = '1';
	memcpy(&largeData[1], g_largeData, klargeDataSize);
	m_events.addEvent(CEvent(m_events.forIScreen().fileChunkSending(), server, (UInt8*)largeData));

	UInt8* transferFinished = new UInt8[2];
	transferFinished[0] = '2';
	transferFinished[1] = '\0';

	m_events.addEvent(CEvent(m_events.forIScreen().fileChunkSending(), server, transferFinished));
}

void
sendFileToClient_getShape(SInt32& x, SInt32& y, SInt32& w, SInt32& h)
{
	x = 0;
	y = 0;
	w = 1;
	h = 1;
}

void
sendFileToClient_getCursorPos(SInt32& x, SInt32& y)
{
	x = 0;
	y = 0;
}
