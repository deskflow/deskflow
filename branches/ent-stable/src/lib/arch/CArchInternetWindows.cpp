/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Bolton Software Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CArchInternetWindows.h"
#include "CArch.h"
#include "Version.h"
#include "XArchWindows.h"

#include <sstream>
#include <Wininet.h>

struct CWinINetUrl {
	CString				m_scheme;
	CString				m_host;
	CString				m_path;
	INTERNET_PORT		m_port;
	DWORD				m_flags;
};

class CWinINetRequest {
public:
	CWinINetRequest(const CString& url);
	~CWinINetRequest();

	CString				send();
	void				openSession();
	void				connect();
	void				openRequest();

private:
	HINTERNET			m_session;
	HINTERNET			m_connect;
	HINTERNET			m_request;
	CWinINetUrl			m_url;
	bool				m_used;
};

//
// CArchInternetWindows
//

CString
CArchInternetWindows::get(const CString& url)
{
	CWinINetRequest request(url);
	return request.send();
}

//
// CWinINetRequest
//

static CWinINetUrl parseUrl(const CString& url);

CWinINetRequest::CWinINetRequest(const CString& url) :
	m_session(NULL),
	m_connect(NULL),
	m_request(NULL),
	m_used(false),
	m_url(parseUrl(url))
{
}

CWinINetRequest::~CWinINetRequest()
{
	if (m_request != NULL) {
		InternetCloseHandle(m_request);
	}

	if (m_connect != NULL) {
		InternetCloseHandle(m_connect);
	}

	if (m_session != NULL) {
		InternetCloseHandle(m_session);
	}
}

CString
CWinINetRequest::send()
{
	if (m_used) {
		throw XArch("class is one time use.");
	}
	m_used = true;

	openSession();
	connect();
	openRequest();
	
	CString headers("Content-Type: text/html");
	if (!HttpSendRequest(m_request, headers.c_str(), (DWORD)headers.length(), NULL, NULL)) {
		throw XArch(new XArchEvalWindows());
	}
	
	std::stringstream result;
	CHAR buffer[1025];
    DWORD read = 0;

	while (InternetReadFile(m_request, buffer, sizeof(buffer) - 1, &read) && (read != 0)) {
		buffer[read] = 0;
		result << buffer;
		read = 0;
	}

	return result.str();
}

void
CWinINetRequest::openSession()
{
	std::stringstream userAgent;
	userAgent << "Synergy ";
	userAgent << kVersion;

    m_session = InternetOpen(
		userAgent.str().c_str(),
		INTERNET_OPEN_TYPE_PRECONFIG,
		NULL,
		NULL,
		NULL);

	if (m_session == NULL) {
		throw XArch(new XArchEvalWindows());
	}
}

void
CWinINetRequest::connect()
{
	m_connect = InternetConnect(
		m_session,
		m_url.m_host.c_str(),
		m_url.m_port,
		NULL,
		NULL,
		INTERNET_SERVICE_HTTP,
		NULL,
		NULL);
	
	if (m_connect == NULL) {
		throw XArch(new XArchEvalWindows());
	}
}

void
CWinINetRequest::openRequest()
{
	m_request = HttpOpenRequest(
		m_connect,
		"GET",
		m_url.m_path.c_str(),
		HTTP_VERSION,
		NULL,
		NULL,
		m_url.m_flags,
		NULL);

	if (m_request == NULL) {
		throw XArch(new XArchEvalWindows());
	}
}

// nb: i tried to use InternetCrackUrl here, but couldn't quite get that to
// work. here's some (less robust) code to split the url into components.
// this works fine with simple urls, but doesn't consider the full url spec.
static CWinINetUrl
parseUrl(const CString& url)
{
	CWinINetUrl parsed;

	size_t schemeEnd = url.find("://");
	size_t hostEnd = url.find('/', schemeEnd + 3);

	parsed.m_scheme = url.substr(0, schemeEnd);
	parsed.m_host = url.substr(schemeEnd + 3, hostEnd - (schemeEnd + 3));
	parsed.m_path = url.substr(hostEnd);

	parsed.m_port = INTERNET_DEFAULT_HTTP_PORT;
	parsed.m_flags = 0;

	if (parsed.m_scheme.find("https") != CString::npos) {
		parsed.m_port = INTERNET_DEFAULT_HTTPS_PORT;
		parsed.m_flags = INTERNET_FLAG_SECURE;
	}

	return parsed;
}
