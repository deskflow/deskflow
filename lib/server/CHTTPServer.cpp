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

#include "CHTTPServer.h"
#include "CConfig.h"
#include "CHTTPProtocol.h"
#include "CServer.h"
#include "XHTTP.h"
#include "IDataSocket.h"
#include "XThread.h"
#include "CLog.h"
#include "stdset.h"
#include "stdsstream.h"

//
// CHTTPServer
//

// maximum size of an HTTP request.  this should be large enough to
// handle any reasonable request but small enough to prevent a
// malicious client from causing us to use too much memory.
const UInt32			CHTTPServer::s_maxRequestSize = 32768;

CHTTPServer::CHTTPServer(
	CServer* server) :
	m_server(server)
{
	// do nothing
}

CHTTPServer::~CHTTPServer()
{
	// do nothing
}

void
CHTTPServer::processRequest(IDataSocket* socket)
{
	assert(socket != NULL);

	CHTTPRequest* request = NULL;
	try {
		// parse request
		request = CHTTPProtocol::readRequest(
								socket->getInputStream(), s_maxRequestSize);
		if (request == NULL) {
			throw XHTTP(400);
		}

		// if absolute uri then strip off scheme and host
		if (request->m_uri[0] != '/') {
			CString::size_type n = request->m_uri.find('/');
			if (n == CString::npos) {
				throw XHTTP(404);
			}
			request->m_uri = request->m_uri.substr(n);
		}

		// prepare reply
		CHTTPReply reply;
		reply.m_majorVersion = request->m_majorVersion;
		reply.m_minorVersion = request->m_minorVersion;
		reply.m_status       = 200;
		reply.m_reason       = "OK";
		reply.m_method       = request->m_method;

		// process
		doProcessRequest(*request, reply);

		// send reply
		CHTTPProtocol::reply(socket->getOutputStream(), reply);
		LOG((CLOG_INFO "HTTP reply %d for %s %s", reply.m_status, request->m_method.c_str(), request->m_uri.c_str()));

		// clean up
		delete request;
	}
	catch (XHTTP& e) {
		LOG((CLOG_WARN "returning HTTP error %d %s for %s", e.getStatus(), e.getReason().c_str(), (request != NULL) ? request->m_uri.c_str() : "<unknown>"));

		// clean up
		delete request;

		// return error
		CHTTPReply reply;
		reply.m_majorVersion = 1;
		reply.m_minorVersion = 0;
		reply.m_status       = e.getStatus();
		reply.m_reason       = e.getReason();
		reply.m_method       = "GET";
// FIXME -- use a nicer error page
		reply.m_headers.push_back(std::make_pair(CString("Content-Type"),
												CString("text/plain")));
		reply.m_body         = e.getReason();
		e.addHeaders(reply);
		CHTTPProtocol::reply(socket->getOutputStream(), reply);
	}
	catch (...) {
		// ignore other exceptions
		RETHROW_XTHREAD
	}
}

void
CHTTPServer::doProcessRequest(CHTTPRequest& request, CHTTPReply& reply)
{
	// switch based on uri
	if (request.m_uri == "/editmap") {
		if (request.m_method == "GET" || request.m_method == "HEAD") {
			doProcessGetEditMap(request, reply);
			reply.m_headers.push_back(std::make_pair(
								CString("Content-Type"),
									CString("text/html")));
		}
		else if (request.m_method == "POST") {
			doProcessPostEditMap(request, reply);
			reply.m_headers.push_back(std::make_pair(
								CString("Content-Type"),
									CString("text/html")));
		}
		else {
			throw XHTTPAllow("GET, HEAD, POST");
		}
	}
	else {
		// unknown page
		throw XHTTP(404);
	}
}

void
CHTTPServer::doProcessGetEditMap(CHTTPRequest& /*request*/, CHTTPReply& reply)
{
	static const char* s_editMapProlog1 =
	"<html>\r\n"
	"<head>\r\n"
	" <title>Synergy -- Edit Screens</title>\r\n"
	"</head>\r\n"
	"<body>\r\n"
	" <form method=\"POST\" action=\"editmap\" "
		"enctype=\"multipart/form-data\">\r\n"
	"  <input type=hidden name=\"size\" value=\"";
	static const char* s_editMapProlog2 =
	"\">\r\n";
	static const char* s_editMapEpilog =
	"  <input type=submit name=\"submit\">\r\n"
	"  <input type=reset>\r\n"
	"  <br>\r\n"
	" </form>\r\n"
	"</body>\r\n"
	"</html>\r\n";
	static const char* s_editMapTableProlog =
	"<table border=\"1\" cellspacing=\"0\" cellpadding=\"0\"><tr><td>"
	" <table border=\"0\" cellspacing=\"2\" cellpadding=\"6\">\r\n";
	static const char* s_editMapTableEpilog =
	" </table>"
	"</td></tr></table>\r\n";
	static const char* s_editMapRowProlog =
	"<tr align=\"center\" valign=\"top\">\r\n";
	static const char* s_editMapRowEpilog =
	"</tr>\r\n";
	static const char* s_editMapScreenDummy =
	"<td>";
	static const char* s_editMapScreenPrimary =
	"<td bgcolor=\"#2222288\"><input type=\"text\" readonly value=\"";
	static const char* s_editMapScreenLive =
	"<td bgcolor=\"#cccccc\"><input type=\"text\" value=\"";
	static const char* s_editMapScreenDead =
	"<td><input type=\"text\" value=\"";
	static const char* s_editMapScreenLiveDead1 =
	"\" size=\"16\" maxlength=\"64\" name=\"";
	static const char* s_editMapScreenLiveDead2 =
	"\">";
	static const char* s_editMapScreenEnd =
	"</td>\r\n";

	std::ostringstream s;

	// convert screen map into a temporary screen map
	CScreenArray screens;
	{
		CConfig config;
		m_server->getConfig(&config);
		screens.convertFrom(config);
		// FIXME -- note to user if config couldn't be exactly represented
	}

	// insert blank columns and rows around array (to allow the user
	// to insert new screens)
	screens.insertColumn(0);
	screens.insertColumn(screens.getWidth());
	screens.insertRow(0);
	screens.insertRow(screens.getHeight());

	// get array size
	const SInt32 w = screens.getWidth();
	const SInt32 h = screens.getHeight();

	// construct reply
	reply.m_body += s_editMapProlog1;
	s << w << "x" << h;
	reply.m_body += s.str();
	reply.m_body += s_editMapProlog2;

	// add screen map for editing
	const CString primaryName = m_server->getPrimaryScreenName();
	reply.m_body += s_editMapTableProlog;
	for (SInt32 y = 0; y < h; ++y) {
		reply.m_body += s_editMapRowProlog;
		for (SInt32 x = 0; x < w; ++x) {
			s.str("");
			if (!screens.isAllowed(x, y) && screens.get(x, y) != primaryName) {
				s << s_editMapScreenDummy;
			}
			else {
				if (!screens.isSet(x, y)) {
					s << s_editMapScreenDead;
				}
				else if (screens.get(x, y) == primaryName) {
					s << s_editMapScreenPrimary;
				}
				else {
					s << s_editMapScreenLive;
				}
				s << screens.get(x, y) <<
								s_editMapScreenLiveDead1 <<
								"n" << x << "x" << y <<
								s_editMapScreenLiveDead2;
			}
			s << s_editMapScreenEnd;
			reply.m_body += s.str();
		}
		reply.m_body += s_editMapRowEpilog;
	}
	reply.m_body += s_editMapTableEpilog;

	reply.m_body += s_editMapEpilog;
}

void
CHTTPServer::doProcessPostEditMap(CHTTPRequest& request, CHTTPReply& reply)
{
	typedef std::vector<CString> ScreenArray;
	typedef std::set<CString> ScreenSet;

	// parse the result
	CHTTPProtocol::CFormParts parts;
	if (!CHTTPProtocol::parseFormData(request, parts)) {
		LOG((CLOG_WARN "editmap: cannot parse form data"));
		throw XHTTP(400);
	}

	try {
		std::ostringstream s;

		// convert post data into a temporary screen map.  also check
		// that no screen name is invalid or used more than once.
		SInt32 w, h;
		CHTTPProtocol::CFormParts::iterator index = parts.find("size");
		if (index == parts.end() ||
			!parseXY(index->second, w, h) ||
			w <= 0 || h <= 0) {
			LOG((CLOG_WARN "editmap: cannot parse size or size is invalid"));
			throw XHTTP(400);
		}
		ScreenSet screenNames;
		CScreenArray screens;
		screens.resize(w, h);
		for (SInt32 y = 0; y < h; ++y) {
			for (SInt32 x = 0; x < w; ++x) {
				// find part
				s.str("");
				s << "n" << x << "x" << y;
				index = parts.find(s.str());
				if (index == parts.end()) {
					// FIXME -- screen is missing.  error?
					continue;
				}

				// skip blank names
				const CString& name = index->second;
				if (name.empty()) {
					continue;
				}

				// check name.  name must be legal and must not have
				// already been seen.
				if (screenNames.count(name)) {
					// FIXME -- better error message
					LOG((CLOG_WARN "editmap: duplicate name %s", name.c_str()));
					throw XHTTP(400);
				}
				// FIXME -- check that name is legal

				// save name.  if we've already seen the name then
				// report an error.
				screens.set(x, y, name);
				screenNames.insert(name);
			}
		}

		// if new map is invalid then return error.  map is invalid if:
		//   there are no screens, or
		//   the screens are not 4-connected.
		if (screenNames.empty()) {
			// no screens
			// FIXME -- need better no screens
			LOG((CLOG_WARN "editmap: no screens"));
			throw XHTTP(400);
		}
		if (!screens.isValid()) {
			// FIXME -- need better unconnected screens error
			LOG((CLOG_WARN "editmap: unconnected screens"));
			throw XHTTP(400);
		}

		// convert temporary screen map into a regular map
		CConfig config;
		m_server->getConfig(&config);
		screens.convertTo(config);

		// set new screen map on server
		m_server->setConfig(config);

		// now reply with current map
		doProcessGetEditMap(request, reply);
	}
	catch (XHTTP&) {
		// FIXME -- construct a more meaningful error?
		throw;
	}
}

bool
CHTTPServer::parseXY(const CString& xy, SInt32& x, SInt32& y)
{
	std::istringstream s(xy);
	char delimiter;
	s >> x;
	s.get(delimiter);
	s >> y;
	return (!!s && delimiter == 'x');
}


//
// CHTTPServer::CScreenArray
//

CHTTPServer::CScreenArray::CScreenArray() :
	m_w(0),
	m_h(0)
{
	// do nothing
}

CHTTPServer::CScreenArray::~CScreenArray()
{
	// do nothing
}

void
CHTTPServer::CScreenArray::resize(SInt32 w, SInt32 h)
{
	m_screens.clear();
	m_screens.resize(w * h);
	m_w = w;
	m_h = h;
}

void
CHTTPServer::CScreenArray::insertRow(SInt32 i)
{
	assert(i >= 0 && i <= m_h);

	CNames newScreens;
	newScreens.resize(m_w * (m_h + 1));

	for (SInt32 y = 0; y < i; ++y) {
		for (SInt32 x = 0; x < m_w; ++x) {
			newScreens[x + y * m_w] = m_screens[x + y * m_w];
		}
	}
	for (SInt32 y = i; y < m_h; ++y) {
		for (SInt32 x = 0; x < m_w; ++x) {
			newScreens[x + (y + 1) * m_w] = m_screens[x + y * m_w];
		}
	}

	m_screens.swap(newScreens);
	++m_h;
}

void
CHTTPServer::CScreenArray::insertColumn(SInt32 i)
{
	assert(i >= 0 && i <= m_w);

	CNames newScreens;
	newScreens.resize((m_w + 1) * m_h);

	for (SInt32 y = 0; y < m_h; ++y) {
		for (SInt32 x = 0; x < i; ++x) {
			newScreens[x + y * (m_w + 1)] = m_screens[x + y * m_w];
		}
		for (SInt32 x = i; x < m_w; ++x) {
			newScreens[(x + 1) + y * (m_w + 1)] = m_screens[x + y * m_w];
		}
	}

	m_screens.swap(newScreens);
	++m_w;
}

void
CHTTPServer::CScreenArray::eraseRow(SInt32 i)
{
	assert(i >= 0 && i < m_h);

	CNames newScreens;
	newScreens.resize(m_w * (m_h - 1));

	for (SInt32 y = 0; y < i; ++y) {
		for (SInt32 x = 0; x < m_w; ++x) {
			newScreens[x + y * m_w] = m_screens[x + y * m_w];
		}
	}
	for (SInt32 y = i + 1; y < m_h; ++y) {
		for (SInt32 x = 0; x < m_w; ++x) {
			newScreens[x + (y - 1) * m_w] = m_screens[x + y * m_w];
		}
	}

	m_screens.swap(newScreens);
	--m_h;
}

void
CHTTPServer::CScreenArray::eraseColumn(SInt32 i)
{
	assert(i >= 0 && i < m_w);

	CNames newScreens;
	newScreens.resize((m_w - 1) * m_h);

	for (SInt32 y = 0; y < m_h; ++y) {
		for (SInt32 x = 0; x < m_w; ++x) {
			newScreens[x + y * (m_w - 1)] = m_screens[x + y * m_w];
		}
		for (SInt32 x = i + 1; x < m_w; ++x) {
			newScreens[(x - 1) + y * (m_w - 1)] = m_screens[x + y * m_w];
		}
	}

	m_screens.swap(newScreens);
	--m_w;
}

void
CHTTPServer::CScreenArray::rotateRows(SInt32 i)
{
	// nothing to do if no rows
	if (m_h == 0) {
		return;
	}

	// convert to canonical form
	if (i < 0) {
		i = m_h - ((-i) % m_h);
	}
	else {
		i %= m_h;
	}
	if (i == 0 || i == m_h) {
		return;
	}

	while (i > 0) {
		// rotate one row
		for (SInt32 x = 0; x < m_w; ++x) {
			CString tmp = m_screens[x];
			for (SInt32 y = 1; y < m_h; ++y) {
				m_screens[x + (y - 1) * m_w] = m_screens[x + y * m_w];
			}
			m_screens[x + (m_h - 1) * m_w] = tmp;
		}
	}
}

void
CHTTPServer::CScreenArray::rotateColumns(SInt32 i)
{
	// nothing to do if no columns
	if (m_h == 0) {
		return;
	}

	// convert to canonical form
	if (i < 0) {
		i = m_w - ((-i) % m_w);
	}
	else {
		i %= m_w;
	}
	if (i == 0 || i == m_w) {
		return;
	}

	while (i > 0) {
		// rotate one column
		for (SInt32 y = 0; y < m_h; ++y) {
			CString tmp = m_screens[0 + y * m_w];
			for (SInt32 x = 1; x < m_w; ++x) {
				m_screens[x - 1 + y * m_w] = m_screens[x + y * m_w];
			}
			m_screens[m_w - 1 + y * m_w] = tmp;
		}
	}
}

void
CHTTPServer::CScreenArray::remove(SInt32 x, SInt32 y)
{
	set(x, y, CString());
}

void
CHTTPServer::CScreenArray::set(SInt32 x, SInt32 y, const CString& name)
{
	assert(x >= 0 && x < m_w);
	assert(y >= 0 && y < m_h);

	m_screens[x + y * m_w] = name;
}

bool
CHTTPServer::CScreenArray::isAllowed(SInt32 x, SInt32 y) const
{
	assert(x >= 0 && x < m_w);
	assert(y >= 0 && y < m_h);

	if (x > 0 && !m_screens[(x - 1) + y * m_w].empty()) {
		return true;
	}
	if (x < m_w - 1 && !m_screens[(x + 1) + y * m_w].empty()) {
		return true;
	}
	if (y > 0 && !m_screens[x + (y - 1) * m_w].empty()) {
		return true;
	}
	if (y < m_h - 1 && !m_screens[x + (y + 1) * m_w].empty()) {
		return true;
	}
	return false;
}

bool
CHTTPServer::CScreenArray::isSet(SInt32 x, SInt32 y) const
{
	assert(x >= 0 && x < m_w);
	assert(y >= 0 && y < m_h);

	return !m_screens[x + y * m_w].empty();
}

CString
CHTTPServer::CScreenArray::get(SInt32 x, SInt32 y) const
{
	assert(x >= 0 && x < m_w);
	assert(y >= 0 && y < m_h);

	return m_screens[x + y * m_w];
}

bool
CHTTPServer::CScreenArray::find(const CString& name,
				SInt32& xOut, SInt32& yOut) const
{
	for (SInt32 y = 0; y < m_h; ++y) {
		for (SInt32 x = 0; x < m_w; ++x) {
			if (m_screens[x + y * m_w] == name) {
				xOut = x;
				yOut = y;
				return true;
			}
		}
	}
	return false;
}

bool
CHTTPServer::CScreenArray::isValid() const
{
	SInt32 count = 0, isolated = 0;
	for (SInt32 y = 0; y < m_h; ++y) {
		for (SInt32 x = 0; x < m_w; ++x) {
			if (isSet(x, y)) {
				++count;
				if (!isAllowed(x, y)) {
					++isolated;
				}
			}
		}
	}
	return (count <= 1 || isolated == 0);
}

bool
CHTTPServer::CScreenArray::convertFrom(const CConfig& config)
{
	typedef std::set<CString> ScreenSet;

	// insert the first screen
	CConfig::const_iterator index = config.begin();
	if (index == config.end()) {
		// no screens
		resize(0, 0);
		return true;
	}
	CString name = *index;
	resize(1, 1);
	set(0, 0, name);

	// flood fill state
	CNames screenStack;
	ScreenSet doneSet;

	// put all but the first screen on the stack
	// note -- if all screens are 4-connected then we can skip this
	while (++index != config.end()) {
		screenStack.push_back(*index);
	}

	// put the first screen on the stack last so we process it first
	screenStack.push_back(name);

	// perform a flood fill using the stack as the seeds
	while (!screenStack.empty()) {
		// get next screen from stack
		CString name = screenStack.back();
		screenStack.pop_back();

		// skip screen if we've seen it before
		if (doneSet.count(name) > 0) {
			continue;
		}

		// add this screen to doneSet so we don't process it again
		doneSet.insert(name);

		// find the screen.  if it's not found then not all of the
		// screens are 4-connected.  discard disconnected screens.
		SInt32 x, y;
		if (!find(name, x, y)) {
			continue;
		}

		// insert the screen's neighbors
		// FIXME -- handle edge wrapping
		CString neighbor;
		neighbor = config.getNeighbor(name, kLeft);
		if (!neighbor.empty() && doneSet.count(neighbor) == 0) {
			// insert left neighbor, adding a column if necessary
			if (x == 0 || get(x - 1, y) != neighbor) {
				++x;
				insertColumn(x - 1);
				set(x - 1, y, neighbor);
			}
			screenStack.push_back(neighbor);
		}
		neighbor = config.getNeighbor(name, kRight);
		if (!neighbor.empty() && doneSet.count(neighbor) == 0) {
			// insert right neighbor, adding a column if necessary
			if (x == m_w - 1 || get(x + 1, y) != neighbor) {
				insertColumn(x + 1);
				set(x + 1, y, neighbor);
			}
			screenStack.push_back(neighbor);
		}
		neighbor = config.getNeighbor(name, kTop);
		if (!neighbor.empty() && doneSet.count(neighbor) == 0) {
			// insert top neighbor, adding a row if necessary
			if (y == 0 || get(x, y - 1) != neighbor) {
				++y;
				insertRow(y - 1);
				set(x, y - 1, neighbor);
			}
			screenStack.push_back(neighbor);
		}
		neighbor = config.getNeighbor(name, kBottom);
		if (!neighbor.empty() && doneSet.count(neighbor) == 0) {
			// insert bottom neighbor, adding a row if necessary
			if (y == m_h - 1 || get(x, y + 1) != neighbor) {
				insertRow(y + 1);
				set(x, y + 1, neighbor);
			}
			screenStack.push_back(neighbor);
		}
	}

	// check symmetry
	// FIXME -- handle edge wrapping
	for (index = config.begin(); index != config.end(); ++index) {
		const CString& name = *index;
		SInt32 x, y;
		if (!find(name, x, y)) {
			return false;
		}

		CString neighbor;
		neighbor = config.getNeighbor(name, kLeft);
		if ((x == 0 && !neighbor.empty()) ||
			(x > 0 && get(x - 1, y) != neighbor)) {
			return false;
		}

		neighbor = config.getNeighbor(name, kRight);
		if ((x == m_w - 1 && !neighbor.empty()) ||
			(x < m_w - 1 && get(x + 1, y) != neighbor)) {
			return false;
		}

		neighbor = config.getNeighbor(name, kTop);
		if ((y == 0 && !neighbor.empty()) ||
			(y > 0 && get(x, y - 1) != neighbor)) {
			return false;
		}

		neighbor = config.getNeighbor(name, kBottom);
		if ((y == m_h - 1 && !neighbor.empty()) ||
			(y < m_h - 1 && get(x, y + 1) != neighbor)) {
			return false;
		}
	}

	return true;
}

void
CHTTPServer::CScreenArray::convertTo(CConfig& config) const
{
	config.removeAllScreens();

	// add screens and find smallest box containing all screens
	SInt32 x0 = m_w, x1 = 0, y0 = m_h, y1 = 0;
	for (SInt32 y = 0; y < m_h; ++y) {
		for (SInt32 x = 0; x < m_w; ++x) {
			if (isSet(x, y)) {
				config.addScreen(get(x, y));
				if (x < x0) {
					x0 = x;
				}
				if (x > x1) {
					x1 = x;
				}
				if (y < y0) {
					y0 = y;
				}
				if (y > y1) {
					y1 = y;
				}
			}

		}
	}

	// make connections between screens
	// FIXME -- add support for wrapping
	// FIXME -- mark topmost and leftmost screens
	for (SInt32 y = 0; y < m_h; ++y) {
		for (SInt32 x = 0; x < m_w; ++x) {
			if (!isSet(x, y)) {
				continue;
			}
			if (x > x0 && isSet(x - 1, y)) {
				config.connect(get(x, y), kLeft, get(x - 1, y));
			}
			if (x < x1 && isSet(x + 1, y)) {
				config.connect(get(x, y), kRight, get(x + 1, y));
			}
			if (y > y0 && isSet(x, y - 1)) {
				config.connect(get(x, y), kTop, get(x, y - 1));
			}
			if (y < y1 && isSet(x, y + 1)) {
				config.connect(get(x, y), kBottom, get(x, y + 1));
			}
		}
	}
}
