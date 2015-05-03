/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si Ltd.
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

#include "SecureSocket.h"

#include "net/TSocketMultiplexerMethodJob.h"
#include "net/TCPSocket.h"
#include "mt/Lock.h"
#include "arch/XArch.h"
#include "base/Log.h"

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <cstring>
#include <cstdlib>
#include <memory>
#include <fstream>

//
// SecureSocket
//

#define MAX_ERROR_SIZE 65535

static const char kFingerprintDirName[] = "SSL/Fingerprints";
//static const char kFingerprintLocalFilename[] = "Local.txt";
static const char kFingerprintTrustedServersFilename[] = "TrustedServers.txt";
//static const char kFingerprintTrustedClientsFilename[] = "TrustedClients.txt";

struct Ssl {
	SSL_CTX*	m_context;
	SSL*		m_ssl;
};

SecureSocket::SecureSocket(
		IEventQueue* events,
		SocketMultiplexer* socketMultiplexer) :
	TCPSocket(events, socketMultiplexer),
	m_secureReady(false)
{
}

SecureSocket::SecureSocket(
		IEventQueue* events,
		SocketMultiplexer* socketMultiplexer,
		ArchSocket socket) :
	TCPSocket(events, socketMultiplexer, socket),
	m_secureReady(false)
{
}

SecureSocket::~SecureSocket()
{
	if (m_ssl->m_ssl != NULL) {
		SSL_shutdown(m_ssl->m_ssl);

		SSL_free(m_ssl->m_ssl);
		m_ssl->m_ssl = NULL;
	}
	if (m_ssl->m_context != NULL) {
		SSL_CTX_free(m_ssl->m_context);
		m_ssl->m_context = NULL;
	}

	delete m_ssl;
}

void
SecureSocket::close()
{
	SSL_shutdown(m_ssl->m_ssl);

	TCPSocket::close();
}

void
SecureSocket::secureConnect()
{
	setJob(new TSocketMultiplexerMethodJob<SecureSocket>(
			this, &SecureSocket::serviceConnect,
			getSocket(), isReadable(), isWritable()));
}

void
SecureSocket::secureAccept()
{
	setJob(new TSocketMultiplexerMethodJob<SecureSocket>(
			this, &SecureSocket::serviceAccept,
			getSocket(), isReadable(), isWritable()));
}

UInt32
SecureSocket::secureRead(void* buffer, UInt32 n)
{
	int r = 0;
	if (m_ssl->m_ssl != NULL) {
		LOG((CLOG_DEBUG2 "reading secure socket"));
		r = SSL_read(m_ssl->m_ssl, buffer, n);
		
		bool fatal, retry;
		checkResult(r, fatal, retry);
		
		if (retry) {
			r = 0;
		}
	}

	return r > 0 ? (UInt32)r : 0;
}

UInt32
SecureSocket::secureWrite(const void* buffer, UInt32 n)
{
	int r = 0;
	if (m_ssl->m_ssl != NULL) {
		LOG((CLOG_DEBUG2 "writing secure socket"));
		r = SSL_write(m_ssl->m_ssl, buffer, n);
		
		bool fatal, retry;
		checkResult(r, fatal, retry);

		if (retry) {
			r = 0;
		}
	}

	return r > 0 ? (UInt32)r : 0;
}

bool
SecureSocket::isSecureReady()
{
	return m_secureReady;
}

void
SecureSocket::initSsl(bool server)
{
	m_ssl = new Ssl();
	m_ssl->m_context = NULL;
	m_ssl->m_ssl = NULL;

	initContext(server);
}

bool
SecureSocket::loadCertificates(String& filename)
{
	if (filename.empty()) {
		showError("ssl certificate is not specified");
		return false;
	}
	else {
		std::ifstream file(filename.c_str());
		bool exist = file.good();
		file.close();

		if (!exist) {
			String errorMsg("ssl certificate doesn't exist: ");
			errorMsg.append(filename);
			showError(errorMsg.c_str());
			return false;
		}
	}

	int r = 0;
	r = SSL_CTX_use_certificate_file(m_ssl->m_context, filename.c_str(), SSL_FILETYPE_PEM);
	if (r <= 0) {
		showError("could not use ssl certificate");
		return false;
	}

	r = SSL_CTX_use_PrivateKey_file(m_ssl->m_context, filename.c_str(), SSL_FILETYPE_PEM);
	if (r <= 0) {
		showError("could not use ssl private key");
		return false;
	}

	r = SSL_CTX_check_private_key(m_ssl->m_context);
	if (!r) {
		showError("could not verify ssl private key");
		return false;
	}

	return true;
}

void
SecureSocket::initContext(bool server)
{
	SSL_library_init();

	const SSL_METHOD* method;
 
	// load & register all cryptos, etc.
	OpenSSL_add_all_algorithms();

	// load all error messages
	SSL_load_error_strings();

	// SSLv23_method uses TLSv1, with the ability to fall back to SSLv3
	if (server) {
		method = SSLv23_server_method();
	}
	else {
		method = SSLv23_client_method();
	}
	
	// create new context from method
	SSL_METHOD* m = const_cast<SSL_METHOD*>(method);
	m_ssl->m_context = SSL_CTX_new(m);

	// drop SSLv3 support
	SSL_CTX_set_options(m_ssl->m_context, SSL_OP_NO_SSLv3);

	if (m_ssl->m_context == NULL) {
		showError();
	}
}

void
SecureSocket::createSSL()
{
	// I assume just one instance is needed
	// get new SSL state with context
	if (m_ssl->m_ssl == NULL) {
		m_ssl->m_ssl = SSL_new(m_ssl->m_context);
	}
}

bool
SecureSocket::secureAccept(int socket)
{
	createSSL();

	// set connection socket to SSL state
	SSL_set_fd(m_ssl->m_ssl, socket);
	
	LOG((CLOG_DEBUG2 "accepting secure socket"));
	int r = SSL_accept(m_ssl->m_ssl);
	
	bool fatal, retry;
	checkResult(r, fatal, retry);

	if (fatal) {
		// tell user and sleep so the socket isn't hammered.
		LOG((CLOG_ERR "failed to accept secure socket"));
		LOG((CLOG_INFO "client connection may not be secure"));
		ARCH->sleep(1);
	}

	m_secureReady = !retry;
	if (m_secureReady) {
		LOG((CLOG_INFO "accepted secure socket"));
	}

	return retry;
}

bool
SecureSocket::secureConnect(int socket)
{
	createSSL();

	// attach the socket descriptor
	SSL_set_fd(m_ssl->m_ssl, socket);
	
	LOG((CLOG_DEBUG2 "connecting secure socket"));
	int r = SSL_connect(m_ssl->m_ssl);
	
	bool fatal, retry;
	checkResult(r, fatal, retry);

	if (fatal) {
		LOG((CLOG_ERR "failed to connect secure socket"));
		LOG((CLOG_INFO "server connection may not be secure"));
		return false;
	}

	m_secureReady = !retry;

	if (m_secureReady) {
		if (verifyCertFingerprint()) {
			LOG((CLOG_INFO "connected to secure socket"));
			if (!showCertificate()) {
				disconnect();
			}
		}
		else {
			LOG((CLOG_ERR "failed to verify server certificate fingerprint"));
			disconnect();
		}
	}

	return retry;
}

bool
SecureSocket::showCertificate()
{
	X509* cert;
	char* line;
 
	// get the server's certificate
	cert = SSL_get_peer_certificate(m_ssl->m_ssl);
	if (cert != NULL) {
		line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
		LOG((CLOG_INFO "server ssl certificate info: %s", line));
		OPENSSL_free(line);
		X509_free(cert);
	}
	else {
		showError("server has no ssl certificate");
		return false;
	}

	return true;
}

void
SecureSocket::checkResult(int n, bool& fatal, bool& retry)
{
	// ssl errors are a little quirky. the "want" errors are normal and
	// should result in a retry.

	fatal = false;
	retry = false;

	int errorCode = SSL_get_error(m_ssl->m_ssl, n);
	switch (errorCode) {
	case SSL_ERROR_NONE:
		// operation completed
		break;

	case SSL_ERROR_ZERO_RETURN:
		// connection closed
		LOG((CLOG_DEBUG2 "SSL connection has been closed"));
		break;

	case SSL_ERROR_WANT_READ:
	case SSL_ERROR_WANT_WRITE:
	case SSL_ERROR_WANT_CONNECT:
	case SSL_ERROR_WANT_ACCEPT:
		LOG((CLOG_DEBUG2 "need to retry the same SSL function"));
		retry = true;
		break;

	case SSL_ERROR_SYSCALL:
		LOG((CLOG_ERR "some secure socket I/O error occurred"));
		if (ERR_peek_error() == 0) {
			if (n == 0) {
				LOG((CLOG_ERR "an EOF violates the protocol"));
			}
			else if (n == -1) {
				// underlying socket I/O reproted an error
				try {
					ARCH->throwErrorOnSocket(getSocket());
				}
				catch (XArchNetwork& e) {
					LOG((CLOG_ERR "%s", e.what()));
				}
			}
		}

		fatal = true;
		break;

	case SSL_ERROR_SSL:
		LOG((CLOG_ERR "a failure in the SSL library occurred"));
		fatal = true;
		break;

	default:
		LOG((CLOG_ERR "unknown secure socket error"));
		fatal = true;
		break;
	}

	if (fatal) {
		showError();
		disconnect();
	}
}

void
SecureSocket::showError(const char* reason)
{
	if (reason != NULL) {
		LOG((CLOG_ERR "%s", reason));
	}

	String error = getError();
	if (!error.empty()) {
		LOG((CLOG_ERR "%s", error.c_str()));
	}
}

String
SecureSocket::getError()
{
	unsigned long e = ERR_get_error();

	if (e != 0) {
		char error[MAX_ERROR_SIZE];
		ERR_error_string_n(e, error, MAX_ERROR_SIZE);
		return error;
	}
	else {
		return "";
	}
}

void
SecureSocket::disconnect()
{
	sendEvent(getEvents()->forISocket().stopRetry());
	sendEvent(getEvents()->forISocket().disconnected());
	sendEvent(getEvents()->forIStream().inputShutdown());
}

void
SecureSocket::formatFingerprint(String& fingerprint, bool hex, bool separator)
{
	if (hex) {
		// to hexidecimal
		synergy::string::toHex(fingerprint, 2);
	}

	// all uppercase
	synergy::string::uppercase(fingerprint);

	if (separator) {
		// add colon to separate each 2 charactors
		size_t separators = fingerprint.size() / 2;
		for (size_t i = 1; i < separators; i++) {
			fingerprint.insert(i * 3 - 1, ":");
		}
	}
}

bool
SecureSocket::verifyCertFingerprint()
{
	// calculate received certificate fingerprint
	X509 *cert = cert = SSL_get_peer_certificate(m_ssl->m_ssl);
	EVP_MD* tempDigest;
	unsigned char tempFingerprint[EVP_MAX_MD_SIZE];
	unsigned int tempFingerprintLen;
	tempDigest = (EVP_MD*)EVP_sha1();
	int digestResult = X509_digest(cert, tempDigest, tempFingerprint, &tempFingerprintLen);

	if (digestResult <= 0) {
		LOG((CLOG_ERR "failed to calculate fingerprint, digest result: %d", digestResult));
		return false;
	}

	// format fingerprint into hexdecimal format with colon separator
	String fingerprint(reinterpret_cast<char*>(tempFingerprint), tempFingerprintLen);
	formatFingerprint(fingerprint);
	LOG((CLOG_NOTE "server fingerprint: %s", fingerprint.c_str()));

	String trustedServersFilename;
	trustedServersFilename = synergy::string::sprintf(
		"%s/%s/%s",
		ARCH->getProfileDirectory().c_str(),
		kFingerprintDirName,
		kFingerprintTrustedServersFilename);

	// check if this fingerprint exist
	String fileLine;
	std::ifstream file;
	file.open(trustedServersFilename.c_str());

	bool isValid = false;
	while (!file.eof() && file.is_open()) {
		getline(file,fileLine);
		if (!fileLine.empty()) {
			if (fileLine.compare(fingerprint) == 0) {
				isValid = true;
				break;
			}
		}
	}

	file.close();
	return isValid;
}

ISocketMultiplexerJob*
SecureSocket::serviceConnect(ISocketMultiplexerJob* job,
				bool, bool write, bool error)
{
	Lock lock(&getMutex());

	bool retry = true;
#ifdef SYSAPI_WIN32
	retry = secureConnect(static_cast<int>(getSocket()->m_socket));
#elif SYSAPI_UNIX
	retry = secureConnect(getSocket()->m_fd);
#endif

	return retry ? job : newJob();
}

ISocketMultiplexerJob*
SecureSocket::serviceAccept(ISocketMultiplexerJob* job,
				bool, bool write, bool error)
{
	Lock lock(&getMutex());

	bool retry = true;
#ifdef SYSAPI_WIN32
	retry = secureAccept(static_cast<int>(getSocket()->m_socket));
#elif SYSAPI_UNIX
	retry = secureAccept(getSocket()->m_fd);
#endif

	return retry ? job : newJob();
}
