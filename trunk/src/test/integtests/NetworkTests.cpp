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

// TODO: fix, tests failing intermittently on mac.
#ifndef WINAPI_CARBON

#include <gtest/gtest.h>
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <iostream>
#include <stdio.h>

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
#include "CTestEventQueue.h"
#include "server/CMockInputFilter.h"
#include "TMethodJob.h"
#include "CThread.h"
#include "CFileChunker.h"

using namespace std;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::Invoke;

#define TEST_PORT 24803
#define TEST_HOST "localhost"

const size_t kMockDataSize = 1024 * 1024 * 10; // 10MB
const UInt16 kMockDataChunkIncrement = 1024; // 1KB
const char* kMockFilename = "NetworkTests.mock";
const size_t kMockFileSize = 1024 * 1024 * 10; // 10MB

void getScreenShape(SInt32& x, SInt32& y, SInt32& w, SInt32& h);
void getCursorPos(SInt32& x, SInt32& y);
CString intToString(size_t i);
UInt8* newMockData(size_t size);
void createFile(fstream& file, const char* filename, size_t size);

class NetworkTests : public ::testing::Test
{
public:
	NetworkTests() :
		m_mockData(NULL),
		m_mockDataSize(0),
		m_mockFileSize(0)
	{
		m_mockData = newMockData(kMockDataSize);
		createFile(m_mockFile, kMockFilename, kMockFileSize);
	}

	~NetworkTests()
	{
		remove(kMockFilename);
		delete[] m_mockData;
	}

	void				sendMockData(void* eventTarget);
	
	void				sendToClient_mockData_handleClientConnected(const CEvent&, void* vlistener);
	void				sendToClient_mockData_fileRecieveCompleted(const CEvent&, void*);
	
	void				sendToClient_mockFile_handleClientConnected(const CEvent&, void* vlistener);
	void				sendToClient_mockFile_fileRecieveCompleted(const CEvent& event, void*);
	
	void				sendToServer_mockData_handleClientConnected(const CEvent&, void* vlistener);
	void				sendToServer_mockData_fileRecieveCompleted(const CEvent& event, void*);

	void				sendToServer_mockFile_handleClientConnected(const CEvent&, void* vlistener);
	void				sendToServer_mockFile_fileRecieveCompleted(const CEvent& event, void*);
	
public:
	CTestEventQueue		m_events;
	UInt8*				m_mockData;
	size_t				m_mockDataSize;
	fstream				m_mockFile;
	size_t				m_mockFileSize;
};

TEST_F(NetworkTests, sendToClient_mockData)
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
			this, &NetworkTests::sendToClient_mockData_handleClientConnected, &listener));

	ON_CALL(serverConfig, isScreen(_)).WillByDefault(Return(true));
	ON_CALL(serverConfig, getInputFilter()).WillByDefault(Return(&serverInputFilter));
	
	CServer server(serverConfig, &primaryClient, &serverScreen, &m_events, true);
	server.m_mock = true;
	listener.setServer(&server);

	// client
	NiceMock<CMockScreen> clientScreen;
	CSocketMultiplexer clientSocketMultiplexer;
	CTCPSocketFactory* clientSocketFactory = new CTCPSocketFactory(&m_events, &clientSocketMultiplexer);
	
	ON_CALL(clientScreen, getShape(_, _, _, _)).WillByDefault(Invoke(getScreenShape));
	ON_CALL(clientScreen, getCursorPos(_, _)).WillByDefault(Invoke(getCursorPos));

	CClient client(&m_events, "stub", serverAddress, clientSocketFactory, NULL, &clientScreen, cryptoOptions, true);
		
	m_events.adoptHandler(
		m_events.forIScreen().fileRecieveCompleted(), &client,
		new TMethodEventJob<NetworkTests>(
			this, &NetworkTests::sendToClient_mockData_fileRecieveCompleted));

	client.connect();

	m_events.initQuitTimeout(10);
	m_events.loop();
	m_events.removeHandler(m_events.forCClientListener().connected(), &listener);
	m_events.removeHandler(m_events.forIScreen().fileRecieveCompleted(), &client);
	m_events.cleanupQuitTimeout();
}

TEST_F(NetworkTests, sendToClient_mockFile)
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
			this, &NetworkTests::sendToClient_mockFile_handleClientConnected, &listener));

	ON_CALL(serverConfig, isScreen(_)).WillByDefault(Return(true));
	ON_CALL(serverConfig, getInputFilter()).WillByDefault(Return(&serverInputFilter));
	
	CServer server(serverConfig, &primaryClient, &serverScreen, &m_events, true);
	server.m_mock = true;
	listener.setServer(&server);

	// client
	NiceMock<CMockScreen> clientScreen;
	CSocketMultiplexer clientSocketMultiplexer;
	CTCPSocketFactory* clientSocketFactory = new CTCPSocketFactory(&m_events, &clientSocketMultiplexer);
	
	ON_CALL(clientScreen, getShape(_, _, _, _)).WillByDefault(Invoke(getScreenShape));
	ON_CALL(clientScreen, getCursorPos(_, _)).WillByDefault(Invoke(getCursorPos));

	CClient client(&m_events, "stub", serverAddress, clientSocketFactory, NULL, &clientScreen, cryptoOptions, true);
		
	m_events.adoptHandler(
		m_events.forIScreen().fileRecieveCompleted(), &client,
		new TMethodEventJob<NetworkTests>(
			this, &NetworkTests::sendToClient_mockFile_fileRecieveCompleted));

	client.connect();

	m_events.initQuitTimeout(10);
	m_events.loop();
	m_events.removeHandler(m_events.forCClientListener().connected(), &listener);
	m_events.removeHandler(m_events.forIScreen().fileRecieveCompleted(), &client);
	m_events.cleanupQuitTimeout();
}

TEST_F(NetworkTests, sendToServer_mockData)
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

	ON_CALL(serverConfig, isScreen(_)).WillByDefault(Return(true));
	ON_CALL(serverConfig, getInputFilter()).WillByDefault(Return(&serverInputFilter));
	
	CServer server(serverConfig, &primaryClient, &serverScreen, &m_events, true);
	server.m_mock = true;
	listener.setServer(&server);

	// client
	NiceMock<CMockScreen> clientScreen;
	CSocketMultiplexer clientSocketMultiplexer;
	CTCPSocketFactory* clientSocketFactory = new CTCPSocketFactory(&m_events, &clientSocketMultiplexer);
	
	ON_CALL(clientScreen, getShape(_, _, _, _)).WillByDefault(Invoke(getScreenShape));
	ON_CALL(clientScreen, getCursorPos(_, _)).WillByDefault(Invoke(getCursorPos));

	CClient client(&m_events, "stub", serverAddress, clientSocketFactory, NULL, &clientScreen, cryptoOptions, true);
	
	m_events.adoptHandler(
		m_events.forCClientListener().connected(), &listener,
		new TMethodEventJob<NetworkTests>(
			this, &NetworkTests::sendToServer_mockData_handleClientConnected, &client));

	m_events.adoptHandler(
		m_events.forIScreen().fileRecieveCompleted(), &server,
		new TMethodEventJob<NetworkTests>(
			this, &NetworkTests::sendToServer_mockData_fileRecieveCompleted));

	client.connect();

	m_events.initQuitTimeout(10);
	m_events.loop();
	m_events.removeHandler(m_events.forCClientListener().connected(), &listener);
	m_events.removeHandler(m_events.forIScreen().fileRecieveCompleted(), &server);
	m_events.cleanupQuitTimeout();
}

TEST_F(NetworkTests, sendToServer_mockFile)
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

	ON_CALL(serverConfig, isScreen(_)).WillByDefault(Return(true));
	ON_CALL(serverConfig, getInputFilter()).WillByDefault(Return(&serverInputFilter));
	
	CServer server(serverConfig, &primaryClient, &serverScreen, &m_events, true);
	server.m_mock = true;
	listener.setServer(&server);

	// client
	NiceMock<CMockScreen> clientScreen;
	CSocketMultiplexer clientSocketMultiplexer;
	CTCPSocketFactory* clientSocketFactory = new CTCPSocketFactory(&m_events, &clientSocketMultiplexer);
	
	ON_CALL(clientScreen, getShape(_, _, _, _)).WillByDefault(Invoke(getScreenShape));
	ON_CALL(clientScreen, getCursorPos(_, _)).WillByDefault(Invoke(getCursorPos));

	CClient client(&m_events, "stub", serverAddress, clientSocketFactory, NULL, &clientScreen, cryptoOptions, true);
	
	m_events.adoptHandler(
		m_events.forCClientListener().connected(), &listener,
		new TMethodEventJob<NetworkTests>(
			this, &NetworkTests::sendToServer_mockFile_handleClientConnected, &client));

	m_events.adoptHandler(
		m_events.forIScreen().fileRecieveCompleted(), &server,
		new TMethodEventJob<NetworkTests>(
			this, &NetworkTests::sendToServer_mockFile_fileRecieveCompleted));

	client.connect();

	m_events.initQuitTimeout(10);
	m_events.loop();
	m_events.removeHandler(m_events.forCClientListener().connected(), &listener);
	m_events.removeHandler(m_events.forIScreen().fileRecieveCompleted(), &server);
	m_events.cleanupQuitTimeout();
}

void 
NetworkTests::sendToClient_mockData_handleClientConnected(const CEvent&, void* vlistener)
{
	CClientListener* listener = reinterpret_cast<CClientListener*>(vlistener);
	CServer* server = listener->getServer();

	CClientProxy* client = listener->getNextClient();
	if (client == NULL) {
		throw runtime_error("client is null");
	}

	CBaseClientProxy* bcp = reinterpret_cast<CBaseClientProxy*>(client);
	server->adoptClient(bcp);
	server->setActive(bcp);

	sendMockData(server);
}

void 
NetworkTests::sendToClient_mockData_fileRecieveCompleted(const CEvent& event, void*)
{
	CClient* client = reinterpret_cast<CClient*>(event.getTarget());
	EXPECT_TRUE(client->isReceivedFileSizeValid());

	m_events.raiseQuitEvent();
}

void 
NetworkTests::sendToClient_mockFile_handleClientConnected(const CEvent&, void* vlistener)
{
	CClientListener* listener = reinterpret_cast<CClientListener*>(vlistener);
	CServer* server = listener->getServer();

	CClientProxy* client = listener->getNextClient();
	if (client == NULL) {
		throw runtime_error("client is null");
	}

	CBaseClientProxy* bcp = reinterpret_cast<CBaseClientProxy*>(client);
	server->adoptClient(bcp);
	server->setActive(bcp);

	server->sendFileToClient(kMockFilename);
}

void 
NetworkTests::sendToClient_mockFile_fileRecieveCompleted(const CEvent& event, void*)
{
	CClient* client = reinterpret_cast<CClient*>(event.getTarget());
	EXPECT_TRUE(client->isReceivedFileSizeValid());

	m_events.raiseQuitEvent();
}

void 
NetworkTests::sendToServer_mockData_handleClientConnected(const CEvent&, void* vclient)
{
	CClient* client = reinterpret_cast<CClient*>(vclient);
	sendMockData(client);
}

void 
NetworkTests::sendToServer_mockData_fileRecieveCompleted(const CEvent& event, void*)
{
	CServer* server = reinterpret_cast<CServer*>(event.getTarget());
	EXPECT_TRUE(server->isReceivedFileSizeValid());

	m_events.raiseQuitEvent();
}

void 
NetworkTests::sendToServer_mockFile_handleClientConnected(const CEvent&, void* vclient)
{
	CClient* client = reinterpret_cast<CClient*>(vclient);
	client->sendFileToServer(kMockFilename);
}

void 
NetworkTests::sendToServer_mockFile_fileRecieveCompleted(const CEvent& event, void*)
{
	CServer* server = reinterpret_cast<CServer*>(event.getTarget());
	EXPECT_TRUE(server->isReceivedFileSizeValid());

	m_events.raiseQuitEvent();
}

void 
NetworkTests::sendMockData(void* eventTarget)
{
	// send first message (file size)
	CString size = intToString(kMockDataSize);
	size_t sizeLength = size.size();
	CFileChunker::CFileChunk* sizeMessage = new CFileChunker::CFileChunk(sizeLength + 2);
	char* chunkData = sizeMessage->m_chunk;

	chunkData[0] = kFileStart;
	memcpy(&chunkData[1], size.c_str(), sizeLength);
	chunkData[sizeLength + 1] = '\0';
	m_events.addEvent(CEvent(m_events.forIScreen().fileChunkSending(), eventTarget, sizeMessage));

	// send chunk messages with incrementing chunk size
	size_t lastSize = 0;
	size_t sentLength = 0;
	while (true) {
		size_t chunkSize = lastSize + kMockDataChunkIncrement;

		// make sure we don't read too much from the mock data.
		if (sentLength + chunkSize > kMockDataSize) {
			chunkSize = kMockDataSize - sentLength;
		}

		// first byte is the chunk mark, last is \0
		CFileChunker::CFileChunk* fileChunk = new CFileChunker::CFileChunk(chunkSize + 2);
		char* chunkData = fileChunk->m_chunk;

		chunkData[0] = kFileChunk;
		memcpy(&chunkData[1], &m_mockData[sentLength], chunkSize);
		chunkData[chunkSize + 1] = '\0';
		m_events.addEvent(CEvent(m_events.forIScreen().fileChunkSending(), eventTarget, fileChunk));

		sentLength += chunkSize;
		lastSize = chunkSize;

		if (sentLength == kMockDataSize) {
			break;
		}

	}
	
	// send last message
	CFileChunker::CFileChunk* transferFinished = new CFileChunker::CFileChunk(2);
	chunkData = transferFinished->m_chunk;

	chunkData[0] = kFileEnd;
	chunkData[1] = '\0';
	m_events.addEvent(CEvent(m_events.forIScreen().fileChunkSending(), eventTarget, transferFinished));
}

UInt8*
newMockData(size_t size)
{
	UInt8* buffer = new UInt8[size];

	UInt8* data = buffer;
	const UInt8 head[] = "mock head... ";
	size_t headSize = sizeof(head) - 1;
	const UInt8 tail[] = "... mock tail";
	size_t tailSize = sizeof(tail) - 1;
	const UInt8 synergyRocks[] = "synergy\0 rocks! ";
	size_t synergyRocksSize = sizeof(synergyRocks) - 1;

	memcpy(data, head, headSize);
	data += headSize;

	size_t times = (size - headSize - tailSize) / synergyRocksSize;
	for (SInt32 i = 0; i < times; ++i) {
		memcpy(data, synergyRocks, synergyRocksSize);
		data += synergyRocksSize;
	}

	size_t remainder = (size - headSize - tailSize) % synergyRocksSize;
	if (remainder != 0) {
		memset(data, '.', remainder);
		data += remainder;
	}

	memcpy(data, tail, tailSize);
	return buffer;
}

void
createFile(fstream& file, const char* filename, size_t size)
{
	UInt8* buffer = newMockData(size);

	file.open(filename, ios::out | ios::binary);
	if (!file.is_open()) {
		throw runtime_error("file not open");
	}

	file.write(reinterpret_cast<char*>(buffer), size);
	file.close();

	delete[] buffer;
}

void
getScreenShape(SInt32& x, SInt32& y, SInt32& w, SInt32& h)
{
	x = 0;
	y = 0;
	w = 1;
	h = 1;
}

void
getCursorPos(SInt32& x, SInt32& y)
{
	x = 0;
	y = 0;
}

CString
intToString(size_t i)
{
	stringstream ss;
	ss << i;
	return ss.str();
}

#endif // WINAPI_CARBON
