/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2015 Synergy Si Ltd.
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

#include "SecureSocket.h"

#include "net/TCPSocket.h"
#include "arch/XArch.h"
#include "base/Log.h"

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <cstring>
#include <cstdlib>
#include <memory>

//
// SecureSocket
//

#define MAX_ERROR_SIZE 65535

struct Ssl {
	SSL_CTX*	m_context;
	SSL*		m_ssl;
};

SecureSocket::SecureSocket(
		IEventQueue* events,
		SocketMultiplexer* socketMultiplexer) :
	TCPSocket(events, socketMultiplexer),
	m_ready(false)
{
}

SecureSocket::SecureSocket(
		IEventQueue* events,
		SocketMultiplexer* socketMultiplexer,
		ArchSocket socket) :
	TCPSocket(events, socketMultiplexer, socket),
	m_ready(false)
{
}

SecureSocket::~SecureSocket()
{
	if (m_ssl->m_ssl != NULL) {
		SSL_free(m_ssl->m_ssl);
	}
	if (m_ssl->m_context != NULL) {
		SSL_CTX_free(m_ssl->m_context);
	}

	delete[] m_error;
}

UInt32
SecureSocket::read(void* buffer, UInt32 n)
{
	bool retry = false;
	int r = 0;
	if (m_ssl != NULL) {
		r = SSL_read(m_ssl->m_ssl, buffer, n);
		retry = checkResult(r);
		if (retry) {
			r = 0;
		}
	}

	return r > 0 ? (UInt32)r : 0;
}

void
SecureSocket::write(const void* buffer, UInt32 n)
{
	bool retry = false;
	int r = 0;
	if (m_ssl != NULL) {
		r = SSL_write(m_ssl->m_ssl, buffer, n);
		retry = checkResult(r);
		if (retry) {
			r = 0;
		}
	}
}

bool
SecureSocket::isReady() const
{
	return m_ready;
}

void
SecureSocket::connectSecureSocket()
{
#ifdef SYSAPI_WIN32
	secureConnect(static_cast<int>(getSocket()->m_socket));
#elif SYSAPI_UNIX
	secureConnect(getSocket()->m_fd);
#endif
}

void
SecureSocket::acceptSecureSocket()
{
#ifdef SYSAPI_WIN32
	secureAccept(static_cast<int>(getSocket()->m_socket));
#elif SYSAPI_UNIX
	secureAccept(getSocket()->m_fd);
#endif
}

void
SecureSocket::initSsl(bool server)
{
	m_ssl = new Ssl();
	m_ssl->m_context = NULL;
	m_ssl->m_ssl = NULL;
	m_error = new char[MAX_ERROR_SIZE];

	initContext(server);
}

void
SecureSocket::onConnected()
{
	TCPSocket::onConnected();

	connectSecureSocket();
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

	if (server) {
		// create new server-method instance
		method = SSLv3_server_method();
	}
	else {
		// create new client-method instance
		method = SSLv3_client_method();
	}
	
	// create new context from method
	m_ssl->m_context = SSL_CTX_new(method);
	if (m_ssl->m_context == NULL) {
		showError();
	}
}

void
SecureSocket::loadCertificates(const char* filename)
{
	int r = 0;
	r = SSL_CTX_use_certificate_file(m_ssl->m_context, filename, SSL_FILETYPE_PEM);
	if (r <= 0) {
		showError();
	}

	r = SSL_CTX_use_PrivateKey_file(m_ssl->m_context, filename, SSL_FILETYPE_PEM);
	if (r <= 0) {
		showError();
	}

	//verify private key
	r = SSL_CTX_check_private_key(m_ssl->m_context);
	if (!r) {
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

void
SecureSocket::secureAccept(int socket)
{
	createSSL();

	// set connection socket to SSL state
	SSL_set_fd(m_ssl->m_ssl, socket);

	// do SSL-protocol accept
	int r = SSL_accept(m_ssl->m_ssl);

	bool retry = checkResult(r);
	while (retry) {
		ARCH->sleep(.5f);
		LOG((CLOG_INFO "secureAccept sleep .5s"));
		r = SSL_accept(m_ssl->m_ssl);
		retry = checkResult(r);
	}

	m_ready = true;
}

void
SecureSocket::secureConnect(int socket)
{
	createSSL();

	// attach the socket descriptor
	SSL_set_fd(m_ssl->m_ssl, socket);

	int r = SSL_connect(m_ssl->m_ssl);

	bool retry = checkResult(r);
	while (retry) {
		ARCH->sleep(.5f);
		r = SSL_connect(m_ssl->m_ssl);
		retry = checkResult(r);
	}

	m_ready= true;
	showCertificate();
}

void
SecureSocket::showCertificate()
{
	X509* cert;
	char* line;
 
	// get the server's certificate
	cert = SSL_get_peer_certificate(m_ssl->m_ssl);
	if (cert != NULL) {
		LOG((CLOG_INFO "server certificate"));
		line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
		LOG((CLOG_INFO "subject: %s", line));
		OPENSSL_free(line);
		line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
		LOG((CLOG_INFO "issuer: %s", line));
		OPENSSL_free(line);
		X509_free(cert);
	}
	else {
		LOG((CLOG_INFO "no certificates"));
	}
}

bool
SecureSocket::checkResult(int n)
{
	bool retry = false;
	int errorCode = SSL_get_error(m_ssl->m_ssl, n);
	
	switch (errorCode) {
	case SSL_ERROR_NONE:
		break;

	case SSL_ERROR_WANT_READ:
		LOG((CLOG_DEBUG2 "secure socket error: SSL_ERROR_WANT_READ"));
		retry = true;
		break;

	case SSL_ERROR_WANT_WRITE:
		LOG((CLOG_DEBUG2 "secure socket error: SSL_ERROR_WANT_WRITE"));
		retry = true;
		break;

	case SSL_ERROR_WANT_CONNECT:
		LOG((CLOG_DEBUG2 "secure socket error: SSL_ERROR_WANT_CONNECT"));
		retry = true;
		break;

	case SSL_ERROR_WANT_ACCEPT:
		LOG((CLOG_DEBUG2 "secure socket error: SSL_ERROR_WANT_ACCEPT"));
		retry = true;
		break;

	case SSL_ERROR_SYSCALL:
		throwError("Secure socket syscall error");
		break;
	case SSL_ERROR_SSL:
		throwError("Secure socket error");
		break;

	default:
		// possible cases: 
		// SSL_ERROR_WANT_X509_LOOKUP, SSL_ERROR_ZERO_RETURN
		showError();
	}

	return retry;
}

void
SecureSocket::showError()
{
	if (getError()) {
		LOG((CLOG_ERR "secure socket error: %s", m_error));
	}
}

void
SecureSocket::throwError(const char* reason)
{
	if (getError()) {
		throw XSecureSocket(synergy::string::sprintf(
			"%s: %s", reason, m_error));
	}
}

bool
SecureSocket::getError()
{
	unsigned long e = ERR_get_error();
	bool errorUpdated = false;

	if (e != 0) {
		ERR_error_string_n(e, m_error, MAX_ERROR_SIZE);
		errorUpdated = true;
	}
	else {
		LOG((CLOG_DEBUG "can not detect any error in secure socket"));
	}
	
	return errorUpdated;
}
