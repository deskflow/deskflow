#include "CClientProxy.h"
#include "IInputStream.h"
#include "IOutputStream.h"

//
// CClientProxy
//

CClientProxy::CClientProxy(IServer* server, const CString& name,
				IInputStream* input, IOutputStream* output) :
	m_server(server),
	m_name(name),
	m_input(input),
	m_output(output)
{
	// do nothing
}

CClientProxy::~CClientProxy()
{
	delete m_output;
	delete m_input;
}

IServer*
CClientProxy::getServer() const
{
	return m_server;
}

IInputStream*
CClientProxy::getInputStream() const
{
	return m_input;
}

IOutputStream*
CClientProxy::getOutputStream() const
{
	return m_output;
}

CString
CClientProxy::getName() const
{
	return m_name;
}
