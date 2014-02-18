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

#include "CArchInternetUnix.h"
#include "Version.h"
#include "XArch.h"
#include "CLog.h"

#include <sstream>
#include <curl/curl.h>

static size_t
curlWriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

CString
CArchInternetUnix::get(const CString& url)
{
	std::string result;
	curl_global_init(CURL_GLOBAL_DEFAULT);
	
	try {
		CURL *curl = curl_easy_init();
		if (curl == NULL) {
			throw XArch("CURL init failed.");
		}
		
		try {
			std::stringstream userAgent;
			userAgent << "Synergy ";
			userAgent << kVersion;
			
			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent.str().c_str());
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
			
			CURLcode code = curl_easy_perform(curl);
			if (code != CURLE_OK) {
				LOG((CLOG_ERR "curl perform error: %s", curl_easy_strerror(code)));
				throw XArch("CURL perform failed.");
			}
			
			curl_easy_cleanup(curl);
		}
		catch (...) {
			curl_easy_cleanup(curl);
			throw;
		}
		
		curl_global_cleanup();
	}
	catch (...) {
		curl_global_cleanup();
		throw;
	}
	
    return result;
}
