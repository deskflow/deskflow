#include "CScreenProxy.h"
#include "ISocket.h"
#include "CMessageSocket.h"
#include "TMethodJob.h"
#include "CTrace.h"
//
// CScreenProxy
//

CScreenProxy::CScreenProxy(const CString& name, ISocket* socket) :
								m_name(name),
								m_socket(socket),
								m_w(0), m_h(0)
{
	assert(!m_name.empty());
	assert(m_socket != NULL);

	m_socket = new CMessageSocket(m_socket);
	m_socket->setReadJob(new TMethodJob<CScreenProxy>(this,
								&CScreenProxy::onRead));
}

CScreenProxy::~CScreenProxy()
{
	delete m_socket;
}

void					CScreenProxy::open(bool isPrimary)
{
	char buf[2];
	memcpy(buf, "\002", 1);
	buf[1] = static_cast<char>(isPrimary ? 1 : 0);
	m_socket->write(buf, sizeof(buf));
}

void					CScreenProxy::close()
{
	char buf[1];
	memcpy(buf, "\003", 1);
	m_socket->write(buf, sizeof(buf));
}

void					CScreenProxy::enterScreen(SInt32 x, SInt32 y)
{
	char buf[9];
	memcpy(buf, "\004", 1);
	buf[1] = static_cast<char>((x >> 24) & 0xff);
	buf[2] = static_cast<char>((x >> 16) & 0xff);
	buf[3] = static_cast<char>((x >> 8) & 0xff);
	buf[4] = static_cast<char>(x & 0xff);
	buf[5] = static_cast<char>((y >> 24) & 0xff);
	buf[6] = static_cast<char>((y >> 16) & 0xff);
	buf[7] = static_cast<char>((y >> 8) & 0xff);
	buf[8] = static_cast<char>(y & 0xff);
	m_socket->write(buf, sizeof(buf));
}

void					CScreenProxy::leaveScreen()
{
	char buf[1];
	memcpy(buf, "\005", 1);
	m_socket->write(buf, sizeof(buf));
}

void					CScreenProxy::warpCursor(SInt32 x, SInt32 y)
{
	char buf[9];
	memcpy(buf, "\020", 1);
	buf[1] = static_cast<char>((x >> 24) & 0xff);
	buf[2] = static_cast<char>((x >> 16) & 0xff);
	buf[3] = static_cast<char>((x >> 8) & 0xff);
	buf[4] = static_cast<char>(x & 0xff);
	buf[5] = static_cast<char>((y >> 24) & 0xff);
	buf[6] = static_cast<char>((y >> 16) & 0xff);
	buf[7] = static_cast<char>((y >> 8) & 0xff);
	buf[8] = static_cast<char>(y & 0xff);
	m_socket->write(buf, sizeof(buf));
}

void					CScreenProxy::setClipboard(const IClipboard*)
{
	// FIXME
}

void					CScreenProxy::onKeyDown(KeyID k, KeyModifierMask m)
{
	char buf[9];
	memcpy(buf, "\007", 1);
	buf[1] = static_cast<char>((k >> 24) & 0xff);
	buf[2] = static_cast<char>((k >> 16) & 0xff);
	buf[3] = static_cast<char>((k >> 8) & 0xff);
	buf[4] = static_cast<char>(k & 0xff);
	buf[5] = static_cast<char>((m >> 24) & 0xff);
	buf[6] = static_cast<char>((m >> 16) & 0xff);
	buf[7] = static_cast<char>((m >> 8) & 0xff);
	buf[8] = static_cast<char>(m & 0xff);
	m_socket->write(buf, sizeof(buf));
}

void					CScreenProxy::onKeyRepeat(
								KeyID k, KeyModifierMask m, SInt32 n)
{
	char buf[13];
	memcpy(buf, "\010", 1);
	buf[1] = static_cast<char>((k >> 24) & 0xff);
	buf[2] = static_cast<char>((k >> 16) & 0xff);
	buf[3] = static_cast<char>((k >> 8) & 0xff);
	buf[4] = static_cast<char>(k & 0xff);
	buf[5] = static_cast<char>((m >> 24) & 0xff);
	buf[6] = static_cast<char>((m >> 16) & 0xff);
	buf[7] = static_cast<char>((m >> 8) & 0xff);
	buf[8] = static_cast<char>(m & 0xff);
	buf[9] = static_cast<char>((n >> 24) & 0xff);
	buf[10] = static_cast<char>((n >> 16) & 0xff);
	buf[11] = static_cast<char>((n >> 8) & 0xff);
	buf[12] = static_cast<char>(n & 0xff);
	m_socket->write(buf, sizeof(buf));
}

void					CScreenProxy::onKeyUp(KeyID k, KeyModifierMask m)
{
	char buf[9];
	memcpy(buf, "\011", 1);
	buf[1] = static_cast<char>((k >> 24) & 0xff);
	buf[2] = static_cast<char>((k >> 16) & 0xff);
	buf[3] = static_cast<char>((k >> 8) & 0xff);
	buf[4] = static_cast<char>(k & 0xff);
	buf[5] = static_cast<char>((m >> 24) & 0xff);
	buf[6] = static_cast<char>((m >> 16) & 0xff);
	buf[7] = static_cast<char>((m >> 8) & 0xff);
	buf[8] = static_cast<char>(m & 0xff);
	m_socket->write(buf, sizeof(buf));
}

void					CScreenProxy::onMouseDown(ButtonID b)
{
	char buf[2];
	memcpy(buf, "\013", 1);
	buf[1] = static_cast<char>(b & 0xff);
	m_socket->write(buf, sizeof(buf));
}

void					CScreenProxy::onMouseUp(ButtonID b)
{
	char buf[2];
	memcpy(buf, "\014", 1);
	buf[1] = static_cast<char>(b & 0xff);
	m_socket->write(buf, sizeof(buf));
}

void					CScreenProxy::onMouseMove(SInt32 x, SInt32 y)
{
	char buf[9];
	memcpy(buf, "\015", 1);
	buf[1] = static_cast<char>((x >> 24) & 0xff);
	buf[2] = static_cast<char>((x >> 16) & 0xff);
	buf[3] = static_cast<char>((x >> 8) & 0xff);
	buf[4] = static_cast<char>(x & 0xff);
	buf[5] = static_cast<char>((y >> 24) & 0xff);
	buf[6] = static_cast<char>((y >> 16) & 0xff);
	buf[7] = static_cast<char>((y >> 8) & 0xff);
	buf[8] = static_cast<char>(y & 0xff);
	m_socket->write(buf, sizeof(buf));
}

void					CScreenProxy::onMouseWheel(SInt32 n)
{
	char buf[5];
	memcpy(buf, "\016", 1);
	buf[1] = static_cast<char>((n >> 24) & 0xff);
	buf[2] = static_cast<char>((n >> 16) & 0xff);
	buf[3] = static_cast<char>((n >> 8) & 0xff);
	buf[4] = static_cast<char>(n & 0xff);
	m_socket->write(buf, sizeof(buf));
}

void					CScreenProxy::onScreenSaver(bool show)
{
	char buf[2];
	memcpy(buf, "\017", 1);
	buf[1] = show ? 1 : 0;
	m_socket->write(buf, sizeof(buf));
}

void					CScreenProxy::onClipboardChanged()
{
	// FIXME
}

CString					CScreenProxy::getName() const
{
	return m_name;
}

void					CScreenProxy::getSize(SInt32* w, SInt32* h) const
{
	assert(w != NULL);
	assert(h != NULL);

	*w = m_w;
	*h = m_h;
}

void					CScreenProxy::getClipboard(IClipboard*) const
{
	// FIXME
}

void					CScreenProxy::onRead()
{
	char buf[512];
	SInt32 n = m_socket->read(buf, sizeof(buf));
	if (n == -1) {
		// FIXME -- disconnect
		TRACE(("hangup"));
	}
	else if (n > 0) {
		switch (buf[0]) {
		  case '\201':
			m_w = static_cast<SInt32>(
								(static_cast<UInt32>(buf[1]) << 24) +
								(static_cast<UInt32>(buf[2]) << 16) +
								(static_cast<UInt32>(buf[3]) << 8) +
								(static_cast<UInt32>(buf[4])     ));
			m_h = static_cast<SInt32>(
								(static_cast<UInt32>(buf[5]) << 24) +
								(static_cast<UInt32>(buf[6]) << 16) +
								(static_cast<UInt32>(buf[7]) << 8) +
								(static_cast<UInt32>(buf[8])     ));
			TRACE(("new size: %dx%d", m_w, m_h));
			break;

		  default:
			TRACE(("unknown message: 0x%02x, %d bytes", buf[0], n));
			break;
		}
	}
}
