#ifndef XHTTP_H
#define XHTTP_H

#include "BasicTypes.h"
#include "CString.h"
#include "XBase.h"

class CHTTPReply;

class XHTTP : public XBase {
public:
	XHTTP(SInt32 statusCode);
	XHTTP(SInt32 statusCode, const CString& reasonPhrase);
	~XHTTP();

	SInt32				getStatus() const;
	CString				getReason() const;

	virtual void		addHeaders(CHTTPReply&) const;

protected:
	virtual CString		getWhat() const throw();

private:
	static const char*	getReason(SInt32 status);

private:
	SInt32				m_status;
	CString				m_reason;
};

class XHTTPAllow : public XHTTP {
public:
	XHTTPAllow(const CString& allowedMethods);
	~XHTTPAllow();

	// XHTTP overrides
	virtual void		addHeaders(CHTTPReply&) const;

private:
	CString				m_allowed;
};

#endif
