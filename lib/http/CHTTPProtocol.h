/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef CHTTPPROTOCOL_H
#define CHTTPPROTOCOL_H

#include "CString.h"
#include "CStringUtil.h"
#include "BasicTypes.h"
#include "stdlist.h"
#include "stdmap.h"
#include "stdvector.h"

class IInputStream;
class IOutputStream;

//! HTTP request type
/*!
This class encapsulates an HTTP request.
*/
class CHTTPRequest {
private:
	typedef std::list<std::pair<CString, CString> > CHeaderList;
public:
	//! Iterator on headers
	/*!
	An iterator on the headers.  Each element is a std::pair;  first is
	the header name as a CString, second is the header value as a CString.
	*/
	typedef CHeaderList::const_iterator const_iterator;

	CHTTPRequest();
	~CHTTPRequest();

	//! @name manipulators
	//@{

	//! Insert header
	/*!
	Add a header by name replacing the existing header, if any.
	Headers are sent in the order they're inserted.  Replacing
	a header does not change its original position in the order.
	*/
	void				insertHeader(const CString& name, const CString& value);

	//! Append header
	/*!
	Append a header.  Equivalent to insertHeader() if the header
	doesn't exist, otherwise it appends a comma and the value to
	the existing header.
	*/
	void				appendHeader(const CString& name, const CString& value);

	//! Remove header
	/*!
	Remove a header by name.  Does nothing if the header doesn't exist.
	*/
	void				eraseHeader(const CString& name);

	//@}
	//! @name accessors
	//@{

	//! Check header existence
	/*!
	Returns true iff the header exists.
	*/
	bool				isHeader(const CString& name) const;

	//! Get header
	/*!
	Get a header by name.  Returns the empty string if the header
	doesn't exist.
	*/
	CString				getHeader(const CString& name) const;

	// headers are iterated in the order they were added.
	//! Get beginning header iterator
	const_iterator		begin() const { return m_headers.begin(); }
	//! Get ending header iterator
	const_iterator		end() const { return m_headers.end(); }

	//@}

public:
	// note -- these members are public for convenience
	//! The HTTP method
	CString				m_method;
	//! The HTTP URI
	CString				m_uri;
	//! The HTTP major version number
	SInt32				m_majorVersion;
	//! The HTTP minor version number
	SInt32				m_minorVersion;
	//! The HTTP body, after transfer decoding
	CString				m_body;

private:
	typedef std::map<CString, CHeaderList::iterator,
							CStringUtil::CaselessCmp> CHeaderMap;

	CHeaderList			m_headers;
	CHeaderMap			m_headerByName;
};

//! HTTP reply type
/*!
This class encapsulates an HTTP reply.
*/
class CHTTPReply {
public:
	//! Header list
	/*!
	The type of the reply header list.  Each pair is the header name
	and value, respectively for first and second.
	*/
	typedef std::vector<std::pair<CString, CString> > CHeaderList;

	// note -- these members are public for convenience
	//! The HTTP major version number
	SInt32				m_majorVersion;
	//! The HTTP minor version number
	SInt32				m_minorVersion;
	//! The HTTP status code
	SInt32				m_status;
	//! The HTTP reason phrase
	CString				m_reason;
	//! The HTTP method
	CString				m_method;
	//! The HTTP headers
	CHeaderList			m_headers;
	//! The HTTP body
	CString				m_body;
};

//! HTTP protocol utilities
/*!
This class provides utility functions for HTTP.
*/
class CHTTPProtocol {
public:
	//! Multipart form parts
	/*!
	Each element is the contents of a multipart form part indexed by
	it's name.
	*/
	typedef std::map<CString, CString> CFormParts;

	//! Read HTTP request
	/*!
	Read and parse an HTTP request.  The result is returned in a
	CHTTPRequest which the client must delete.  Throws an
	XHTTP if there was a parse error.  Throws an XIO exception
	if there was a read error.  If \c maxSize is greater than
	zero and the request is larger than \c maxSize bytes then
	throws XHTTP(413) (request entity too large).
	*/
	static CHTTPRequest*	readRequest(IInputStream*, UInt32 maxSize = 0);

	//! Send HTTP response
	/*!
	Send an HTTP reply.  The Content-Length and Date headers are set
	automatically.
	*/
	static void			reply(IOutputStream*, CHTTPReply&);

	//! Parse multipart form data
	/*!
	Parse a multipart/form-data body into its parts.  Returns true
	iff the entire body was correctly parsed.
	*/
	// FIXME -- name/value pairs insufficient to save part headers
	static bool			parseFormData(const CHTTPRequest&,
							CFormParts& parts);

private:
	static CString		readLine(IInputStream*, CString& tmpBuffer);
	static CString		readBlock(IInputStream*,
							UInt32 numBytes, CString& tmpBuffer);
	static CString		readChunk(IInputStream*, CString& tmpBuffer,
							UInt32* maxSize);
	static void			readHeaders(IInputStream*,
							CHTTPRequest*, bool isFooter,
							CString& tmpBuffer,
							UInt32* maxSize);

	static bool			isValidToken(const CString&);
};

#endif
