#ifndef XHTTP_H
#define XHTTP_H

#include "BasicTypes.h"
#include "XBase.h"

class CHTTPReply;

//! Generic HTTP exception
class XHTTP : public XBase {
public:
	/*!
	Use the HTTP \c statusCode as the failure reason.
	*/
	XHTTP(SInt32 statusCode);
	/*!
	Use the HTTP \c statusCode as the failure reason.  Use \c reasonPhrase
	as the human readable reason for the failure.
	*/
	XHTTP(SInt32 statusCode, const CString& reasonPhrase);
	~XHTTP();

	//@{
	//! @name accessors

	//! Get the HTTP status code
	SInt32				getStatus() const;

	//! Get the reason phrase
	CString				getReason() const;

	//! Modify reply for error
	/*!
	Override to modify an HTTP reply to further describe the error.
	*/
	virtual void		addHeaders(CHTTPReply&) const;

	//@}

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
	/*!
	\c allowedMethods is added as an `Allow' header to a reply in
	addHeaders().
	*/
	XHTTPAllow(const CString& allowedMethods);
	~XHTTPAllow();

	// XHTTP overrides
	virtual void		addHeaders(CHTTPReply&) const;

private:
	CString				m_allowed;
};

#endif
