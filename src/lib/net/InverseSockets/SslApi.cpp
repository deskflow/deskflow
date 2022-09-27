#include "SslApi.h"
#include "SslLogger.h"

#include <memory>
#include <fstream>

#include <base/Log.h>
#include <base/Path.h>
#include <openssl/err.h>

namespace synergy {
namespace ssl {

using AutoX509 = std::unique_ptr<X509, decltype(&X509_free)>;

SslApi::SslApi(bool isServer)
{
    SSL_library_init();
    // load & register all cryptos, etc.
    OpenSSL_add_all_algorithms();
    // load all error messages
    SSL_load_error_strings();
    createContext(isServer);
    SslLogger::logSecureLibInfo();
}

SslApi::~SslApi()
{
    if (m_ssl) {
        SSL_shutdown(m_ssl);
        SSL_free(m_ssl);
        m_ssl = nullptr;
    }

    if (m_context) {
        SSL_CTX_free(m_context);
        m_context = nullptr;
    }
}

int SslApi::read(char* buffer, int size)
{
    auto read = 0;

    if (m_ssl) {
        read = SSL_read(m_ssl, buffer, size);
    }

    return read;
}

int SslApi::write(const char* buffer, int size)
{
    auto wrote = 0;

    if (m_ssl) {
        wrote = SSL_write(m_ssl, buffer, size);
    }

    return wrote;
}

int SslApi::accept(int socket)
{
    int result = 0;

    if (m_ssl) {
        // set connection socket to SSL state
        SSL_set_fd(m_ssl, socket);
        result = SSL_accept(m_ssl);
    }

    return result;
}

int SslApi::connect(int socket)
{
    auto result = 0;

    if (m_ssl) {
        // attach the socket descriptor
        SSL_set_fd(m_ssl, socket);
        result = SSL_connect(m_ssl);
    }

    return result;
}

void SslApi::createSSL()
{
    if (m_ssl == nullptr && m_context != nullptr) {
        m_ssl = SSL_new(m_context);
    }
}

bool SslApi::loadCertificate(const std::string& filename)
{
    bool result = false;

    if (isCertificateExists(filename)) {
        auto r = SSL_CTX_use_certificate_file(m_context, filename.c_str(), SSL_FILETYPE_PEM);
        if (r <= 0) {
            SslLogger::logError("could not use tls certificate");
            return false;
        }

        r = SSL_CTX_use_PrivateKey_file(m_context, filename.c_str(), SSL_FILETYPE_PEM);
        if (r <= 0) {
            SslLogger::logError("could not use tls private key");
            return false;
        }

        r = SSL_CTX_check_private_key(m_context);
        if (!r) {
            SslLogger::logError("could not verify tls private key");
            return false;
        }
    }

    return result;
}

bool SslApi::showCertificate() const
{
    bool result = false;

    if (m_ssl) {
        // get the server's certificate
        AutoX509 cert(SSL_get_peer_certificate(m_ssl), &X509_free);
        if (cert) {
            auto line = X509_NAME_oneline(X509_get_subject_name(cert.get()), nullptr, 0);
            LOG((CLOG_INFO "server tls certificate info: %s", line));
            OPENSSL_free(line);
            result = true;
        }
        else {
            SslLogger::logError("server has no tls certificate");
        }
    }

    return result;
}

std::string SslApi::getFingerprint() const
{
    // calculate received certificate fingerprint
    AutoX509 cert(SSL_get_peer_certificate(m_ssl), &X509_free);
    unsigned int tempFingerprintLen = 0;
    unsigned char tempFingerprint[EVP_MAX_MD_SIZE] = {0};
    int digestResult = X509_digest(cert.get(), EVP_sha256(), tempFingerprint, &tempFingerprintLen);

    if (digestResult <= 0) {
        LOG((CLOG_ERR "failed to calculate fingerprint, digest result: %d", digestResult));
        return "";
    }

    // format fingerprint into hexdecimal format with colon separator
    std::string fingerprint(static_cast<char*>(static_cast<void*>(tempFingerprint)), tempFingerprintLen);
    formatFingerprint(fingerprint);

    return fingerprint;
}

bool SslApi::isTrustedFingerprint(const std::string& fingerprint) const
{
    auto trustedServersFilename = synergy::string::sprintf(
                "%s/SSL/Fingerprints/TrustedServers.txt",
                ARCH->getProfileDirectory().c_str());

    // check if this fingerprint exist
    std::ifstream file;
    file.open(synergy::filesystem::path(trustedServersFilename));

    bool isValid = false;
    if (file.is_open()) {
        while (!file.eof()) {
            std::string fileLine;
            getline(file, fileLine);
            if (!fileLine.empty() && !fileLine.compare(fingerprint)) {
                isValid = true;
                break;
            }
        }
    }
    else {
        LOG((CLOG_ERR "Fail to open trusted fingerprints file: %s", trustedServersFilename.c_str()));
    }

    return (isValid && showCertificate());
}


void SslApi::createContext(bool isServer)
{
    // create new context from method
    if (isServer) {
        m_context = SSL_CTX_new(SSLv23_server_method());
    }
    else {
        m_context = SSL_CTX_new(SSLv23_client_method());
    }
    //Prevent the usage of of all version prior to TLSv1.2 as they are known to be vulnerable
    SSL_CTX_set_options(m_context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);

    if (m_context) {
        m_ssl = SSL_new(m_context);
    }
    else {
        SslLogger::logError();
    }
}

void SslApi::logSecureInfo() const
{
    SslLogger::logSecureCipherInfo(m_ssl);
    SslLogger::logSecureConnectInfo(m_ssl);

}

int SslApi::getErrorCode(int status) const
{
    return SSL_get_error(m_ssl, status);
}

void SslApi::formatFingerprint(std::string &fingerprint) const
{
    // to hexidecimal
    synergy::string::toHex(fingerprint, 2);
    // all uppercase
    synergy::string::uppercase(fingerprint);
    // add colon to separate each 2 charactors
    size_t separators = fingerprint.size() / 2;
    for (size_t i = 1; i < separators; i++) {
        fingerprint.insert(i * 3 - 1, ":");
    }
}

bool SslApi::isCertificateExists(const std::string &filename) const
{
    bool result = (!filename.empty());

    if (result) {
        std::ifstream file(synergy::filesystem::path(filename));
        result = file.good();

        if (!result) {
            std::string errorMsg("tls certificate doesn't exist: ");
            errorMsg.append(filename);
            SslLogger::logError(errorMsg.c_str());
        }
    }
    else {
        SslLogger::logError("tls certificate is not specified");
    }

    return result;
}

} //namespace ssl
} //namespace synergy
