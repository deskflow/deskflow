#include "CServerProtocol.h"
#include "CServerProtocol1_0.h"
#include "ProtocolTypes.h"
#include "IOutputStream.h"
#include <stdio.h>
#include <assert.h>

//
// CServerProtocol
//

CServerProtocol::CServerProtocol(CServer* server, const CString& client,
								IInputStream* input, IOutputStream* output) :
								m_server(server),
								m_client(client),
								m_input(input),
								m_output(output)
{
	assert(m_server != NULL);
	assert(m_input  != NULL);
	assert(m_output != NULL);
}

CServerProtocol::~CServerProtocol()
{
	// do nothing
}

CServer*				CServerProtocol::getServer() const
{
	return m_server;
}

CString					CServerProtocol::getClient() const
{
	return m_client;
}

IInputStream*			CServerProtocol::getInputStream() const
{
	return m_input;
}

IOutputStream*			CServerProtocol::getOutputStream() const
{
	return m_output;
}

IServerProtocol*		CServerProtocol::create(SInt32 major, SInt32 minor,
								CServer* server, const CString& client,
								IInputStream* input, IOutputStream* output)
{
	// disallow invalid version numbers
	if (major < 0 || minor < 0) {
		throw XIncompatibleClient(major, minor);
	}

	// disallow connection from test versions to release versions
	if (major == 0 && kMajorVersion != 0) {
		throw XIncompatibleClient(major, minor);
	}

	// hangup (with error) if version isn't supported
	if (major > kMajorVersion ||
		(major == kMajorVersion && minor > kMinorVersion)) {
		throw XIncompatibleClient(major, minor);
	}

	// create highest version protocol object not higher than the
	// given version.
	return new CServerProtocol1_0(server, client, input, output);
}

