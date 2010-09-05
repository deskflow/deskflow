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

#include "CHTTPProtocol.h"
#include "XHTTP.h"
#include "IInputStream.h"
#include "IOutputStream.h"
#include "CLog.h"
#include "stdsstream.h"
#include <clocale>
#include <ctime>
#include <algorithm>

//
// CHTTPRequest
//

CHTTPRequest::CHTTPRequest()
{
	// do nothing
}

CHTTPRequest::~CHTTPRequest()
{
	// do nothing
}

void
CHTTPRequest::insertHeader(const CString& name, const CString& value)
{
	CHeaderMap::iterator index = m_headerByName.find(name);
	if (index != m_headerByName.end()) {
		index->second->second = value;
	}
	else {
		CHeaderList::iterator pos = m_headers.insert(
							m_headers.end(), std::make_pair(name, value));
		m_headerByName.insert(std::make_pair(name, pos));
	}
}

void
CHTTPRequest::appendHeader(const CString& name, const CString& value)
{
	CHeaderMap::iterator index = m_headerByName.find(name);
	if (index != m_headerByName.end()) {
		index->second->second += ",";
		index->second->second += value;
	}
	else {
		CHeaderList::iterator pos = m_headers.insert(
							m_headers.end(), std::make_pair(name, value));
		m_headerByName.insert(std::make_pair(name, pos));
	}
}

void
CHTTPRequest::eraseHeader(const CString& name)
{
	CHeaderMap::iterator index = m_headerByName.find(name);
	if (index != m_headerByName.end()) {
		m_headers.erase(index->second);
	}
}

bool
CHTTPRequest::isHeader(const CString& name) const
{
	return (m_headerByName.find(name) != m_headerByName.end());
}

CString
CHTTPRequest::getHeader(const CString& name) const
{
	CHeaderMap::const_iterator index = m_headerByName.find(name);
	if (index != m_headerByName.end()) {
		return index->second->second;
	}
	else {
		return CString();
	}
}


//
// CHTTPProtocol
//
					
CHTTPRequest*
CHTTPProtocol::readRequest(IInputStream* stream, UInt32 maxSize)
{
	CString scratch;

	// note if we should limit the request size
	const bool checkSize = (maxSize > 0);

	// parse request line by line
	CHTTPRequest* request = new CHTTPRequest;
	try {
		CString line;

		// read request line.  accept and discard leading empty lines.
		do {
			line = readLine(stream, scratch);
			if (checkSize) {
				if (line.size() + 2 > maxSize) {
					throw XHTTP(413);
				}
				maxSize -= line.size() + 2;
			}
		} while (line.empty());

		// parse request line:  <method> <uri> <version>
		{
			std::istringstream s(line);
			s.exceptions(std::ios::goodbit);
			CString version;
			s >> request->m_method >> request->m_uri >> version;
			if (!s || request->m_uri.empty() || version.find("HTTP/") != 0) {
				LOG((CLOG_DEBUG1 "failed to parse HTTP request line: %s", line.c_str()));
				throw XHTTP(400);
			}

			// parse version
			char dot;
			s.str(version);
			s.clear();
			s.ignore(5);
			s >> request->m_majorVersion;
			s.get(dot);
			s >> request->m_minorVersion;
			if (!s || dot != '.') {
				LOG((CLOG_DEBUG1 "failed to parse HTTP request line: %s", line.c_str()));
				throw XHTTP(400);
			}
		}
		if (!isValidToken(request->m_method)) {
			LOG((CLOG_DEBUG1 "invalid HTTP method: %s", line.c_str()));
			throw XHTTP(400);
		}
		if (request->m_majorVersion < 1 || request->m_minorVersion < 0) {
			LOG((CLOG_DEBUG1 "invalid HTTP version: %s", line.c_str()));
			throw XHTTP(400);
		}

		// parse headers
		readHeaders(stream, request, false, scratch,
								checkSize ? &maxSize : NULL);

		// HTTP/1.1 requests must have a Host header
		if (request->m_majorVersion > 1 ||
			(request->m_majorVersion == 1 && request->m_minorVersion >= 1)) {
			if (request->isHeader("Host") == 0) {
				LOG((CLOG_DEBUG1 "Host header missing"));
				throw XHTTP(400);
			}
		}

		// some methods may not have a body.  ensure that the headers
		// that indicate the body length do not exist for those methods
		// and do exist for others.
		if ((request->isHeader("Transfer-Encoding") ||
			 request->isHeader("Content-Length")) ==
			(request->m_method == "GET" ||
			 request->m_method == "HEAD")) {
			LOG((CLOG_DEBUG1 "HTTP method (%s)/body mismatch", request->m_method.c_str()));
			throw XHTTP(400);
		}

		// prepare to read the body.  the length of the body is
		// determined using, in order:
		//   1. Transfer-Encoding indicates a "chunked" transfer
		//   2. Content-Length is present
		// Content-Length is ignored for "chunked" transfers.
		CString header;
		if (!(header = request->getHeader("Transfer-Encoding")).empty()) {
			// we only understand "chunked" encodings
			if (!CStringUtil::CaselessCmp::equal(header, "chunked")) {
				LOG((CLOG_DEBUG1 "unsupported Transfer-Encoding %s", header.c_str()));
				throw XHTTP(501);
			}

			// chunked encoding
			UInt32 oldSize;
			do {
				oldSize = request->m_body.size();
				request->m_body += readChunk(stream, scratch,
										checkSize ? &maxSize : NULL);
			} while (request->m_body.size() != oldSize);

			// read footer
			readHeaders(stream, request, true, scratch,
								checkSize ? &maxSize : NULL);

			// remove "chunked" from Transfer-Encoding and set the
			// Content-Length.
			std::ostringstream s;
			s << std::dec << request->m_body.size();
			request->eraseHeader("Transfer-Encoding");
			request->insertHeader("Content-Length", s.str());
		}
		else if (!(header = request->getHeader("Content-Length")).empty()) {
			// parse content-length
			UInt32 length;
			{
				std::istringstream s(header);
				s.exceptions(std::ios::goodbit);
				s >> length;
				if (!s) {
					LOG((CLOG_DEBUG1 "cannot parse Content-Length", header.c_str()));
					throw XHTTP(400);
				}
			}

			// check against expected size
			if (checkSize && length > maxSize) {
				throw XHTTP(413);
			}

			// use content length
			request->m_body = readBlock(stream, length, scratch);
			if (request->m_body.size() != length) {
				// length must match size of body
				LOG((CLOG_DEBUG1 "Content-Length/actual length mismatch (%d vs %d)", length, request->m_body.size()));
				throw XHTTP(400);
			}
		}
	}
	catch (...) {
		delete request;
		throw;
	}

	return request;
}

void
CHTTPProtocol::reply(IOutputStream* stream, CHTTPReply& reply)
{
	// suppress body for certain replies
	bool hasBody = true;
	if ((reply.m_status / 100) == 1 ||
		reply.m_status == 204 ||
		reply.m_status == 304) {
		hasBody = false;
	}

	// adjust headers
	for (CHTTPReply::CHeaderList::iterator
								index = reply.m_headers.begin();
								index != reply.m_headers.end(); ) {
		const CString& header = index->first;

		// remove certain headers
		if (CStringUtil::CaselessCmp::equal(header, "Content-Length") ||
			CStringUtil::CaselessCmp::equal(header, "Date") ||
			CStringUtil::CaselessCmp::equal(header, "Transfer-Encoding")) {
			// FIXME -- Transfer-Encoding should be left as-is if
			// not "chunked" and if the version is 1.1 or up.
			index = reply.m_headers.erase(index);
		}

		// keep as-is
		else {
			++index;
		}
	}

	// write reply header
	std::ostringstream s;
	s << "HTTP/" << reply.m_majorVersion << "." <<
					reply.m_minorVersion << " " <<
					reply.m_status << " " <<
					reply.m_reason << "\r\n";

	// get date
	// FIXME -- should use C++ locale stuff but VC++ time_put is broken.
	// FIXME -- double check that VC++ is broken
	char date[30];
	{
		const char* oldLocale = setlocale(LC_TIME, "C");
		time_t t = time(NULL);
#if HAVE_GMTIME_R
		struct tm tm;
		struct tm* tmp = &tm;
		gmtime_r(&t, tmp);
#else
		struct tm* tmp = gmtime(&t);
#endif
		strftime(date, sizeof(date), "%a, %d %b %Y %H:%M:%S GMT", tmp);
		setlocale(LC_TIME, oldLocale);
	}

	// write headers
	s << "Date: " << date << "\r\n";
	for (CHTTPReply::CHeaderList::const_iterator
								index = reply.m_headers.begin();
								index != reply.m_headers.end(); ++index) {
		s << index->first << ": " << index->second << "\r\n";
	}
	if (hasBody) {
		s << "Content-Length: " << reply.m_body.size() << "\r\n";
	}
	s << "Connection: close\r\n";

	// write end of headers
	s << "\r\n";

	// write to stream
	stream->write(s.str().data(), s.str().size());

	// write body.  replies to HEAD method never have a body (though
	// they do have the Content-Length header).
	if (hasBody && reply.m_method != "HEAD") {
		stream->write(reply.m_body.data(), reply.m_body.size());
	}
}

bool
CHTTPProtocol::parseFormData(const CHTTPRequest& request, CFormParts& parts)
{
	static const char formData[]    = "multipart/form-data";
	static const char boundary[]    = "boundary=";
	static const char disposition[] = "Content-Disposition:";
	static const char nameAttr[]    = "name=";
	static const char quote[]       = "\"";

	// find the Content-Type header
	const CString contentType = request.getHeader("Content-Type");
	if (contentType.empty()) {
		// missing required Content-Type header
		return false;
	}

	// parse type
	CString::const_iterator index = std::search(
								contentType.begin(), contentType.end(),
								formData, formData + sizeof(formData) - 1,
								CStringUtil::CaselessCmp::cmpEqual);
	if (index == contentType.end()) {
		// not form-data
		return false;
	}
	index += sizeof(formData) - 1;
	index = std::search(index, contentType.end(),
								boundary, boundary + sizeof(boundary) - 1,
								CStringUtil::CaselessCmp::cmpEqual);
	if (index == contentType.end()) {
		// no boundary
		return false;
	}
	CString delimiter = contentType.c_str() +
								(index - contentType.begin()) +
								sizeof(boundary) - 1;

	// find first delimiter
	const CString& body = request.m_body;
	CString::size_type partIndex = body.find(delimiter);
	if (partIndex == CString::npos) {
		return false;
	}

	// skip over it
	partIndex += delimiter.size();

	// prepend CRLF-- to delimiter
	delimiter = "\r\n--" + delimiter;

	// parse parts until there are no more
	for (;;) {
		// is it the last part?
		if (body.size() >= partIndex + 2 &&
			body[partIndex    ] == '-' &&
			body[partIndex + 1] == '-') {
			// found last part.  ignore trailing data, if any.
			return true;
		}

		// find the end of this part
		CString::size_type nextPart = body.find(delimiter, partIndex);
		if (nextPart == CString::npos) {
			// no terminator
			return false;
		}

		// find end of headers
		CString::size_type endOfHeaders = body.find("\r\n\r\n", partIndex);
		if (endOfHeaders == CString::npos || endOfHeaders > nextPart) {
			// bad part
			return false;
		}
		endOfHeaders += 2;

		// now find Content-Disposition
		index = std::search(body.begin() + partIndex,
								body.begin() + endOfHeaders,
								disposition,
								disposition + sizeof(disposition) - 1,
								CStringUtil::CaselessCmp::cmpEqual);
		if (index == contentType.begin() + endOfHeaders) {
			// bad part
			return false;
		}

		// find the name in the Content-Disposition
		CString::size_type endOfHeader = body.find("\r\n",
								index - body.begin());
		if (endOfHeader >= endOfHeaders) {
			// bad part
			return false;
		}
		index = std::search(index, body.begin() + endOfHeader,
								nameAttr, nameAttr + sizeof(nameAttr) - 1,
								CStringUtil::CaselessCmp::cmpEqual);
		if (index == body.begin() + endOfHeader) {
			// no name
			return false;
		}

		// extract the name
		CString name;
		index += sizeof(nameAttr) - 1;
		if (*index == quote[0]) {
			// quoted name
			++index;
			CString::size_type namePos = index - body.begin();
			index = std::search(index, body.begin() + endOfHeader,
								quote, quote + 1,
								CStringUtil::CaselessCmp::cmpEqual);
			if (index == body.begin() + endOfHeader) {
				// missing close quote
				return false;
			}
			name = body.substr(namePos, index - body.begin() - namePos);
		}
		else {
			// unquoted name
			name = body.substr(index - body.begin(),
								body.find_first_of(" \t\r\n"));
		}

		// save part.  add 2 to endOfHeaders to skip CRLF.
		parts.insert(std::make_pair(name, body.substr(endOfHeaders + 2,
									nextPart - (endOfHeaders + 2))));

		// move to next part
		partIndex = nextPart + delimiter.size();
	}

	// should've found the last delimiter inside the loop but we did not
	return false;	
}

CString
CHTTPProtocol::readLine(IInputStream* stream, CString& tmpBuffer)
{
	// read up to and including a CRLF from stream, using whatever
	// is in tmpBuffer as if it were at the head of the stream.

	for (;;) {
		// scan tmpBuffer for CRLF
		CString::size_type newline = tmpBuffer.find("\r\n");
		if (newline != CString::npos) {
			// copy line without the CRLF
			CString line = tmpBuffer.substr(0, newline);

			// discard line and CRLF from tmpBuffer
			tmpBuffer.erase(0, newline + 2);
			return line;
		}

		// read more from stream
		char buffer[4096];
		UInt32 n = stream->read(buffer, sizeof(buffer), -1.0);
		if (n == 0) {
			// stream is empty.  return what's leftover.
			CString line = tmpBuffer;
			tmpBuffer.erase();
			return line;
		}

		// append stream data
		tmpBuffer.append(buffer, n);
	}
}

CString
CHTTPProtocol::readBlock(IInputStream* stream,
				UInt32 numBytes, CString& tmpBuffer)
{
	CString data;

	// read numBytes from stream, using whatever is in tmpBuffer as
	// if it were at the head of the stream.
	if (tmpBuffer.size() > 0) {
		// ignore stream if there's enough data in tmpBuffer
		if (tmpBuffer.size() >= numBytes) {
			data = tmpBuffer.substr(0, numBytes);
			tmpBuffer.erase(0, numBytes);
			return data;
		}

		// move everything out of tmpBuffer into data
		data = tmpBuffer;
		tmpBuffer.erase();
	}

	// account for bytes read so far
	assert(data.size() < numBytes);
	numBytes -= data.size();

	// read until we have all the requested data
	while (numBytes > 0) {
		// read max(4096, bytes_left) bytes into buffer
		char buffer[4096];
		UInt32 n = sizeof(buffer);
		if (n > numBytes) {
			n = numBytes;
		}
		n = stream->read(buffer, n, -1.0);

		// if stream is empty then return what we've got so far
		if (n == 0) {
			break;
		}

		// append stream data
		data.append(buffer, n);
		numBytes -= n;
	}

	return data;
}

CString
CHTTPProtocol::readChunk(IInputStream* stream,
				CString& tmpBuffer, UInt32* maxSize)
{
	CString line;

	// get chunk header
	line = readLine(stream, tmpBuffer);

	// parse chunk size
	UInt32 size;
	{
		std::istringstream s(line);
		s.exceptions(std::ios::goodbit);
		s >> std::hex >> size;
		if (!s) {
			LOG((CLOG_DEBUG1 "cannot parse chunk size", line.c_str()));
			throw XHTTP(400);
		}
	}
	if (size == 0) {
		return CString();
	}

	// check size
	if (maxSize != NULL) {
		if (line.size() + 2 + size + 2 > *maxSize) {
			throw XHTTP(413);
		}
		maxSize -= line.size() + 2 + size + 2;
	}

	// read size bytes
	CString data = readBlock(stream, size, tmpBuffer);
	if (data.size() != size) {
		LOG((CLOG_DEBUG1 "expected/actual chunk size mismatch", size, data.size()));
		throw XHTTP(400);
	}

	// read an discard CRLF
	line = readLine(stream, tmpBuffer);
	if (!line.empty()) {
		LOG((CLOG_DEBUG1 "missing CRLF after chunk"));
		throw XHTTP(400);
	}

	return data;
}

void
CHTTPProtocol::readHeaders(IInputStream* stream,
				CHTTPRequest* request, bool isFooter,
				CString& tmpBuffer, UInt32* maxSize)
{
	// parse headers.  done with headers when we get a blank line.
	CString name;
	CString line = readLine(stream, tmpBuffer);
	while (!line.empty()) {
		// check size
		if (maxSize != NULL) {
			if (line.size() + 2 > *maxSize) {
				throw XHTTP(413);
			}
			*maxSize -= line.size() + 2;
		}

		// if line starts with space or tab then append it to the
		// previous header.  if there is no previous header then
		// throw.
		if (line[0] == ' ' || line[0] == '\t') {
			if (name.empty()) {
				LOG((CLOG_DEBUG1 "first header is a continuation"));
				throw XHTTP(400);
			}
			request->appendHeader(name, line);
		}

		// line should have the form:  <name>:[<value>]
		else {
			// parse
			CString value;
			std::istringstream s(line);
			s.exceptions(std::ios::goodbit);
			std::getline(s, name, ':');
			if (!s || !isValidToken(name)) {
				LOG((CLOG_DEBUG1 "invalid header: %s", line.c_str()));
				throw XHTTP(400);
			}
			std::getline(s, value);

			// check validity of name
			if (isFooter) {
				// FIXME -- only certain names are allowed in footers
				// but which ones?
			}

			request->appendHeader(name, value);
		}

		// next header
		line = readLine(stream, tmpBuffer);
	}
}

bool
CHTTPProtocol::isValidToken(const CString& token)
{
	return (token.find("()<>@,;:\\\"/[]?={} "
					"\0\1\2\3\4\5\6\7"
					"\10\11\12\13\14\15\16\17"
					"\20\21\22\23\24\25\26\27"
					"\30\31\32\33\34\35\36\37\177") == CString::npos);
}
