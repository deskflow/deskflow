#ifndef CHTTPPROTOCOL_H
#define CHTTPPROTOCOL_H

#include "BasicTypes.h"
#include "CString.h"
#include "stdlist.h"
#include "stdmap.h"
#include "stdvector.h"

class IInputStream;
class IOutputStream;

class CHTTPUtil {
public:
	class CaselessCmp {
	  public:
		bool			operator()(const CString&, const CString&) const;
		static bool		less(const CString&, const CString&);
		static bool		equal(const CString&, const CString&);
		static bool		cmpLess(const CString::value_type&,
								const CString::value_type&);
		static bool		cmpEqual(const CString::value_type&,
								const CString::value_type&);
	};
};

class CHTTPRequest {
public:
	typedef std::list<std::pair<CString, CString> > CHeaderList;
	typedef std::map<CString, CHeaderList::iterator,
								CHTTPUtil::CaselessCmp> CHeaderMap;
	typedef CHeaderList::const_iterator const_iterator;

	CHTTPRequest();
	~CHTTPRequest();

	// manipulators

	// add a header by name.  replaces existing header, if any.
	// headers are sent in the order they're inserted.  replacing
	// a header does not change its original position in the order.
	void				insertHeader(const CString& name, const CString& value);

	// append a header.  equivalent to insertHeader() if the header
	// doesn't exist, otherwise it appends a comma and the value to
	// the existing header.
	void				appendHeader(const CString& name, const CString& value);

	// remove a header by name.  does nothing if no such header.
	void				eraseHeader(const CString& name);

	// accessors

	// returns true iff the header exists
	bool				isHeader(const CString& name) const;

	// get a header by name.  returns the empty string if no such header.
	CString				getHeader(const CString& name) const;

	// get iterator over all headers in the order they were added
	const_iterator		begin() const { return m_headers.begin(); }
	const_iterator		end() const { return m_headers.end(); }

public:
	// note -- these members are public for convenience
	CString				m_method;
	CString				m_uri;
	SInt32				m_majorVersion;
	SInt32				m_minorVersion;
	CString				m_body;

private:
	CHeaderList			m_headers;
	CHeaderMap			m_headerByName;
};

class CHTTPReply {
public:
	typedef std::vector<std::pair<CString, CString> > CHeaderList;

	SInt32				m_majorVersion;
	SInt32				m_minorVersion;
	SInt32				m_status;
	CString				m_reason;
	CString				m_method;

	CHeaderList			m_headers;

	CString				m_body;
};

class CHTTPProtocol {
public:
	// read and parse an HTTP request.  result is returned in a
	// CHTTPRequest which the client must delete.  throws an
	// XHTTP if there was a parse error.  throws an XIO exception
	// if there was a read error.  if maxSize is greater than
	// zero and the request is larger than maxSize bytes then
	// throws XHTTP(413).
	static CHTTPRequest*	readRequest(IInputStream*, UInt32 maxSize = 0);

	// send an HTTP reply on the stream
	static void			reply(IOutputStream*, CHTTPReply&);

	// parse a multipart/form-data body into its parts.  returns true
	// iff the entire body was correctly parsed.
	// FIXME -- name/value pairs insufficient to save part headers
	typedef std::map<CString, CString> CFormParts;
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
