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

#ifndef CHTTPSERVER_H
#define CHTTPSERVER_H

#include "CString.h"
#include "BasicTypes.h"
#include "stdvector.h"

class CServer;
class CConfig;
class CHTTPRequest;
class CHTTPReply;
class IDataSocket;

//! Simple HTTP server
/*!
This class implements a simple HTTP server for interacting with the
synergy server.
*/
class CHTTPServer {
public:
	CHTTPServer(CServer*);
	virtual ~CHTTPServer();

	//! @name manipulators
	//@{

	//! Process HTTP request
	/*!
	Synchronously processes an HTTP request on the given socket.
	*/
	void				processRequest(IDataSocket*);

	//@}

protected:
	//! Process HTTP request
	/*!
	Processes a successfully read HTTP request.  The reply is partially
	filled in (version, method, status (200) and reason (OK)).  This
	method checks the URI and handles the request, filling in the rest
	of the reply.  If the request cannot be satisfied it throws an
	appropriate XHTTP exception.
	*/
	virtual void		doProcessRequest(CHTTPRequest&, CHTTPReply&);

	//! Process request for map
	virtual void		doProcessGetEditMap(CHTTPRequest&, CHTTPReply&);

	//! Process request for changing map
	virtual void		doProcessPostEditMap(CHTTPRequest&, CHTTPReply&);

	//! Parse coordinate string
	static bool			parseXY(const CString&, SInt32& x, SInt32& y);

	//! Screen map helper
	/*!
	This class represents the screen map as a resizable array.  It's
	used to handle map requests.
	*/
	class CScreenArray {
	public:
		CScreenArray();
		~CScreenArray();

		// resize the array.  this also clears all the elements.
		void			resize(SInt32 w, SInt32 h);

		// insert/remove a row/column.  all elements in a new row/column
		// are unset.
		void			insertRow(SInt32 insertedBeforeRow);
		void			insertColumn(SInt32 insertedBeforeColumn);
		void			eraseRow(SInt32 row);
		void			eraseColumn(SInt32 column);

		// rotate rows or columns
		void			rotateRows(SInt32 rowsDown);
		void			rotateColumns(SInt32 columnsDown);

		// remove/set a screen name.  setting an empty name is the
		// same as removing a name.  names are not checked for
		// validity.
		void			remove(SInt32 x, SInt32 y);
		void			set(SInt32 x, SInt32 y, const CString&);

		// convert a CConfig to a CScreenArray.  returns true iff
		// all connections are symmetric and therefore exactly
		// representable by a CScreenArray.
		bool			convertFrom(const CConfig&);

		// accessors

		// get the array size
		SInt32			getWidth() const  { return m_w; }
		SInt32			getHeight() const { return m_h; }

		// returns true iff the cell has a 4-connected neighbor
		bool			isAllowed(SInt32 x, SInt32 y) const;

		// returns true iff the cell has a (non-empty) name
		bool			isSet(SInt32 x, SInt32 y) const;

		// get a screen name
		CString			get(SInt32 x, SInt32 y) const;

		// find a screen by name.  returns true iff found.
		bool			find(const CString&, SInt32& x, SInt32& y) const;

		// return true iff the overall array is valid.  that means
		// just zero or one screen or all screens are 4-connected
		// to other screens.
		bool			isValid() const;

		// convert this to a CConfig
		void			convertTo(CConfig&) const;

	private:
		typedef std::vector<CString> CNames;

		SInt32			m_w, m_h;
		CNames			m_screens;
	};

private:
	CServer*			m_server;
	static const UInt32	s_maxRequestSize;
};

#endif
