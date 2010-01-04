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

#include "CClientApp.h"

CClientApp::CArgs* CClientApp::CArgs::s_instance = NULL;

CClientApp::CClientApp()
{
}

CClientApp::~CClientApp()
{
}

CClientApp::CArgs::CArgs() :
m_pname(NULL),
m_backend(false),
m_restartable(true),
m_daemon(true),
m_yscroll(0),
m_logFilter(NULL),
m_display(NULL),
m_serverAddress(NULL),
m_logFile(NULL)
{
	s_instance = this;
}

CClientApp::CArgs::~CArgs()
{
	s_instance = NULL;
}