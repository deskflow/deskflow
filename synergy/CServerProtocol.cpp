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

CServer*				CServerProtocol::getServer() const throw()
{
	return m_server;
}

CString					CServerProtocol::getClient() const throw()
{
	return m_client;
}

IInputStream*			CServerProtocol::getInputStream() const throw()
{
	return m_input;
}

IOutputStream*			CServerProtocol::getOutputStream() const throw()
{
	return m_output;
}

IServerProtocol*		CServerProtocol::create(SInt32 major, SInt32 minor,
								CServer* server, const CString& client,
								IInputStream* input, IOutputStream* output)
{
	// disallow connection from test versions to release versions
	if (major == 0 && kMajorVersion != 0) {
		output->write(kMsgEIncompatible, sizeof(kMsgEIncompatible) - 1);
		output->flush();
		throw XIncompatibleClient(major, minor);
	}

	// hangup (with error) if version isn't supported
	if (major > kMajorVersion ||
		(major == kMajorVersion && minor > kMinorVersion)) {
		output->write(kMsgEIncompatible, sizeof(kMsgEIncompatible) - 1);
		output->flush();
		throw XIncompatibleClient(major, minor);
	}

	// create highest version protocol object not higher than the
	// given version.
	return new CServerProtocol1_0(server, client, input, output);
}

