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

CString
CArchInternetWindows::get(const CString& url)
{
	std::stringstream userAgent;
	userAgent << "Synergy ";
	userAgent << kVersion;

    HINTERNET session = InternetOpen(
		userAgent.str().c_str(),
		INTERNET_OPEN_TYPE_PRECONFIG,
		NULL, NULL, NULL);

	if (session == NULL) {
		throw XArch(new XArchEvalWindows());
	}

	// InternetCrackUrl didn't seem to work too well, this isn't quite
	// as robust, but it should do just fine for basic URLs.
	size_t schemeEnd = url.find("://");
	size_t hostEnd = url.find('/', schemeEnd + 3);
	CString scheme = url.substr(0, schemeEnd);
	CString host = url.substr(schemeEnd + 3, hostEnd - (schemeEnd + 3));
	CString path = url.substr(hostEnd);

	INTERNET_PORT port = INTERNET_DEFAULT_HTTP_PORT;
	DWORD requestFlags = 0;

	if (scheme.find("https") != CString::npos) {
		port = INTERNET_DEFAULT_HTTPS_PORT;
		requestFlags = INTERNET_FLAG_SECURE;
	}

	HINTERNET connect = InternetConnect(
		session, host.c_str(), port, NULL, NULL,
		INTERNET_SERVICE_HTTP, NULL, NULL);
	
	if (connect == NULL) {
		throw XArch(new XArchEvalWindows());
	}

	HINTERNET request = HttpOpenRequest(
		connect, "GET", path.c_str(),
		HTTP_VERSION, NULL,
		NULL, requestFlags, NULL);

	if (request == NULL) {
		throw XArch(new XArchEvalWindows());
	}

	CString headers("Content-Type: text/html");
	if (!HttpSendRequest(request, headers.c_str(), (DWORD)headers.length(), NULL, NULL)) {
		int error = GetLastError();
		throw XArch(new XArchEvalWindows());
	}
	
	std::stringstream result;
	CHAR buffer[1025];
    DWORD read = 0;

	while (InternetReadFile(request, buffer, sizeof(buffer) - 1, &read) && (read != 0)) {
		buffer[read] = 0;
		result << buffer;
		read = 0;
	}

    InternetCloseHandle(request);
    InternetCloseHandle(connect);
    InternetCloseHandle(session);

	return result.str();
}
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

CString
CArchInternetWindows::get(const CString& url)
{
    return "Hello bob!";
}
