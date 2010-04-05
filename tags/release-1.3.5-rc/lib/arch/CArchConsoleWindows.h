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

#ifndef CARCHCONSOLEWINDOWS_H
#define CARCHCONSOLEWINDOWS_H

#define WIN32_LEAN_AND_MEAN

#include "IArchConsole.h"
#include "IArchMultithread.h"
#include "stddeque.h"
#include <windows.h>

#define ARCH_CONSOLE CArchConsoleWindows

//! Win32 implementation of IArchConsole
class CArchConsoleWindows : public IArchConsole {
public:
	CArchConsoleWindows(void*);
	virtual ~CArchConsoleWindows();

	// IArchConsole overrides
	virtual void		openConsole(const char* title);
	virtual void		closeConsole();
	virtual void		showConsole(bool showIfEmpty);
	virtual void		writeConsole(const char*);
	virtual const char*	getNewlineForConsole();

private:
	void				clearBuffer();
	void				appendBuffer(const char*);
	void				setSize(int width, int height);

	LRESULT				wndProc(HWND, UINT, WPARAM, LPARAM);
	static LRESULT CALLBACK
						staticWndProc(HWND, UINT, WPARAM, LPARAM);
	void				threadMainLoop();
	static void*		threadEntry(void*);

private:
	typedef std::deque<std::string> MessageBuffer;

	static CArchConsoleWindows*	s_instance;
	static HINSTANCE	s_appInstance;

	// multithread data
	CArchMutex			m_mutex;
	CArchCond			m_condVar;
	bool				m_ready;
	CArchThread			m_thread;

	// child thread data
	HWND				m_frame;
	HWND				m_hwnd;
	LONG				m_wChar;
	LONG				m_hChar;
	bool				m_show;

	// messages
	size_t				m_maxLines;
	size_t				m_maxCharacters;
	size_t				m_numCharacters;
	MessageBuffer		m_buffer;
};

#endif
