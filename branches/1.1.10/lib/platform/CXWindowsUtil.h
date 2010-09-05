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

#ifndef CXWINDOWSUTIL_H
#define CXWINDOWSUTIL_H

#include "CString.h"
#include "BasicTypes.h"
#include "stdmap.h"
#include "stdvector.h"
#if X_DISPLAY_MISSING
#	error X11 is required to build synergy
#else
#	include <X11/Xlib.h>
#endif

//! X11 utility functions
class CXWindowsUtil {
public:
	typedef std::vector<KeySym> KeySyms;

	//! Get property
	/*!
	Gets property \c property on \c window.  \b Appends the data to
	\c *data if \c data is not NULL, saves the property type in \c *type
	if \c type is not NULL, and saves the property format in \c *format
	if \c format is not NULL.  If \c deleteProperty is true then the
	property is deleted after being read.
	*/
	static bool			getWindowProperty(Display*,
							Window window, Atom property,
							CString* data, Atom* type,
							SInt32* format, bool deleteProperty);

	//! Set property
	/*!
	Sets property \c property on \c window to \c size bytes of data from
	\c data.
	*/
	static bool			setWindowProperty(Display*,
							Window window, Atom property,
							const void* data, UInt32 size,
							Atom type, SInt32 format);

	//! Get X server time
	/*!
	Returns the current X server time.
	*/
	static Time			getCurrentTime(Display*, Window);

	//! Convert KeySym to UCS-4
	/*!
	Converts a KeySym to the equivalent UCS-4 character.  Returns
	0x0000ffff if the KeySym cannot be mapped.
	*/
	static UInt32		mapKeySymToUCS4(KeySym);

	//! Convert UCS-4 to KeySym
	/*!
	Converts a UCS-4 character to the equivalent KeySym.  Returns
	NoSymbol (0) if the character cannot be mapped.
	*/
	static KeySym		mapUCS4ToKeySym(UInt32);

	//! Decompose a KeySym using dead keys
	/*!
	Decomposes \c keysym into its component keysyms.  All but the last
	decomposed KeySym are dead keys.  Returns true iff the decomposition
	was successful.
	*/
	static bool			decomposeKeySymWithDeadKeys(KeySym keysym,
							KeySyms& decomposed);

	//! Decompose a KeySym using the compose key
	/*!
	Decomposes \c keysym into its component keysyms.  The first key is
	Multi_key and the rest are normal (i.e. not dead) keys.  Returns
	true iff the decomposition was successful.
	*/
	static bool			decomposeKeySymWithCompose(KeySym keysym,
							KeySyms& decomposed);

	//! Convert Atom to its string
	/*!
	Converts \p atom to its string representation.
	*/
	static CString		atomToString(Display*, Atom atom);

	//! Convert several Atoms to a string
	/*!
	Converts each atom in \p atoms to its string representation and
	concatenates the results.
	*/
	static CString		atomsToString(Display* display,
							const Atom* atom, UInt32 num);

	//! X11 error handler
	/*!
	This class sets an X error handler in the c'tor and restores the
	previous error handler in the d'tor.  A lock should only be
	installed while the display is locked by the thread.
	
	CErrorLock() ignores errors
	CErrorLock(bool* flag) sets *flag to true if any error occurs
	*/
	class CErrorLock {
	public:
		//! Error handler type
		typedef void (*ErrorHandler)(Display*, XErrorEvent*, void* userData);

		/*!
		Ignore X11 errors.
		*/
		CErrorLock(Display*);

		/*!
		Set \c *errorFlag if any error occurs.
		*/
		CErrorLock(Display*, bool* errorFlag);

		/*!
		Call \c handler on each error.
		*/
		CErrorLock(Display*, ErrorHandler handler, void* userData);

		~CErrorLock();

	private:
		void			install(ErrorHandler, void*);
		static int		internalHandler(Display*, XErrorEvent*);
		static void		ignoreHandler(Display*, XErrorEvent*, void*);
		static void		saveHandler(Display*, XErrorEvent*, void*);

	private:
		typedef int (*XErrorHandler)(Display*, XErrorEvent*);

		Display*		m_display;
		ErrorHandler	m_handler;
		void*			m_userData;
		XErrorHandler	m_oldXHandler;
		CErrorLock*		m_next;
		static CErrorLock*	s_top;
	};

private:
	class CPropertyNotifyPredicateInfo {
	public:
		Window			m_window;
		Atom			m_property;
	};

	static Bool			propertyNotifyPredicate(Display*,
							XEvent* xevent, XPointer arg);

	static void			initKeyMaps();

private:
	typedef std::map<KeySym, UInt32> CKeySymMap;
	typedef std::map<UInt32, KeySym> CUCS4Map;
	typedef std::map<KeySym, KeySyms> CKeySymsMap;

	static CKeySymMap	s_keySymToUCS4;
	static CUCS4Map		s_UCS4ToKeySym;
	static CKeySymsMap	s_deadKeyDecomposedKeySyms;
	static CKeySymsMap	s_composeDecomposedKeySyms;
};

#endif
