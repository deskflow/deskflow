#ifndef CHTTPPROTOCOL_H
#define CHTTPPROTOCOL_H

#include "BasicTypes.h"
#include "CString.h"
#include <map>
#include <vector>

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
	typedef std::map<CString, UInt32, CHTTPUtil::CaselessCmp> CHeaderMap;
	typedef std::vector<CString> CHeaderList;

	CString				m_method;
	CString				m_uri;
	SInt32				m_majorVersion;
	SInt32				m_minorVersion;

	CHeaderList			m_headers;
	CHeaderMap			m_headerIndexByName;

	CString				m_body;
	// FIXME -- need parts-of-body for POST messages
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
	// XHTTP if there was a parse error.  throws an XIO
	// exception if there was a read error.
	static CHTTPRequest*	readRequest(IInputStream*);

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
	static CString		readChunk(IInputStream*, CString& tmpBuffer);
	static void			readHeaders(IInputStream*,
								CHTTPRequest*, bool isFooter,
								CString& tmpBuffer);

	static bool			isValidToken(const CString&);
};

#endif
