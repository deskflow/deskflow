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

#include "XHTTP.h"
#include "CHTTPProtocol.h"
#include "CStringUtil.h"
#include "stdsstream.h"

//
// XHTTP
//

XHTTP::XHTTP(SInt32 statusCode) :
	XBase(),
	m_status(statusCode),
	m_reason(getReason(statusCode))
{
	// do nothing
}

XHTTP::XHTTP(SInt32 statusCode, const CString& reasonPhrase) :
	XBase(),
	m_status(statusCode),
	m_reason(reasonPhrase)
{
	// do nothing
}

XHTTP::~XHTTP()
{
	// do nothing
}

SInt32
XHTTP::getStatus() const
{
	return m_status;
}

CString
XHTTP::getReason() const
{
	return m_reason;
}

void
XHTTP::addHeaders(CHTTPReply&) const
{
	// do nothing
}

CString
XHTTP::getWhat() const throw()
{
	const char* reason;
	if (m_reason.empty()) {
		reason = getReason(m_status);
	}
	else {
		reason = m_reason.c_str();
	}
	return format("XHTTP", "%{1} %{2}",
								CStringUtil::print("%d", m_status).c_str(),
								reason);
}

const char*
XHTTP::getReason(SInt32 status)
{
	switch (status) {
	case 300: return "Multiple Choices";
	case 301: return "Moved Permanently";
	case 302: return "Moved Temporarily";
	case 303: return "See Other";
	case 304: return "Not Modified";
	case 305: return "Use Proxy";
	case 400: return "Bad Request";
	case 401: return "Unauthorized";
	case 402: return "Payment Required";
	case 403: return "Forbidden";
	case 404: return "Not Found";
	case 405: return "Method Not Allowed";
	case 406: return "Not Acceptable";
	case 407: return "Proxy Authentication Required";
	case 408: return "Request Time-out";
	case 409: return "Conflict";
	case 410: return "Gone";
	case 411: return "Length Required";
	case 412: return "Precondition Failed";
	case 413: return "Request Entity Too Large";
	case 414: return "Request-URI Too Large";
	case 415: return "Unsupported Media Type";
	case 500: return "Internal Server Error";
	case 501: return "Not Implemented";
	case 502: return "Bad Gateway";
	case 503: return "Service Unavailable";
	case 504: return "Gateway Time-out";
	case 505: return "HTTP Version not supported";
	default:  return "";
	}
}


//
// XHTTPAllow
//

XHTTPAllow::XHTTPAllow(const CString& allowedMethods) :
	XHTTP(405),
	m_allowed(allowedMethods)
{
	// do nothing
}

XHTTPAllow::~XHTTPAllow()
{
	// do nothing
}

void
XHTTPAllow::addHeaders(CHTTPReply& reply) const
{
	reply.m_headers.push_back(std::make_pair(CString("Allow"), m_allowed));
}
