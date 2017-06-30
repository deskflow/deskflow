/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2016 Symless Ltd.
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "arch/unix/ArchInternetUnix.h"

#include "arch/XArch.h"
#include "common/Version.h"
#include "base/Log.h"

#include <sstream>
#include <curl/curl.h>

class CurlFacade {
public:
    CurlFacade ();
    ~CurlFacade ();
    String get (const String& url);
    String urlEncode (const String& url);

private:
    CURL* m_curl;
};

//
// ArchInternetUnix
//

String
ArchInternetUnix::get (const String& url) {
    CurlFacade curl;
    return curl.get (url);
}

String
ArchInternetUnix::urlEncode (const String& url) {
    CurlFacade curl;
    return curl.urlEncode (url);
}

//
// CurlFacade
//

static size_t
curlWriteCallback (void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*) userp)->append ((char*) contents, size * nmemb);
    return size * nmemb;
}

CurlFacade::CurlFacade () : m_curl (NULL) {
    CURLcode init = curl_global_init (CURL_GLOBAL_ALL);
    if (init != CURLE_OK) {
        throw XArch ("CURL global init failed.");
    }

    m_curl = curl_easy_init ();
    if (m_curl == NULL) {
        throw XArch ("CURL easy init failed.");
    }
}

CurlFacade::~CurlFacade () {
    if (m_curl != NULL) {
        curl_easy_cleanup (m_curl);
    }

    curl_global_cleanup ();
}

String
CurlFacade::get (const String& url) {
    curl_easy_setopt (m_curl, CURLOPT_URL, url.c_str ());
    curl_easy_setopt (m_curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);

    std::stringstream userAgent;
    userAgent << "Synergy ";
    userAgent << kVersion;
    curl_easy_setopt (m_curl, CURLOPT_USERAGENT, userAgent.str ().c_str ());

    std::string result;
    curl_easy_setopt (m_curl, CURLOPT_WRITEDATA, &result);

    CURLcode code = curl_easy_perform (m_curl);
    if (code != CURLE_OK) {
        LOG ((CLOG_ERR "curl perform error: %s", curl_easy_strerror (code)));
        throw XArch ("CURL perform failed.");
    }

    return result;
}

String
CurlFacade::urlEncode (const String& url) {
    char* resultCStr = curl_easy_escape (m_curl, url.c_str (), 0);

    if (resultCStr == NULL) {
        throw XArch ("CURL escape failed.");
    }

    std::string result (resultCStr);
    curl_free (resultCStr);

    return result;
}
