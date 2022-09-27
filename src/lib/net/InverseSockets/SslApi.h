/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015-2022 Symless Ltd.
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
#pragma once
#include <string>
#include <openssl/ssl.h>

namespace synergy {
namespace ssl {

class SslApi
{
public:
    explicit SslApi(bool isServer = false);
    SslApi(SslApi const &) =delete;
    SslApi& operator=(SslApi const &) =delete;
    ~SslApi();

    int read(char* buffer, int size);
    int write(const char* buffer, int size);
    int accept(int socket);
    int connect(int socket);

    bool loadCertificate(const std::string& filename);
    bool showCertificate() const;
    std::string getFingerprint() const;
    bool isTrustedFingerprint(const std::string& fingerprint) const;

    void logSecureInfo() const;
    int getErrorCode(int status) const;

private:
    void createSSL();
    void formatFingerprint(std::string& fingerprint) const;
    bool isCertificateExists(const std::string& filename) const;
    void createContext(bool isServer = false);

    SSL*        m_ssl = nullptr;
    SSL_CTX*    m_context = nullptr;
};

} //namespace ssl
} //namespace synergy
