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

#include "CServerApp.h"

CServerApp::CArgs* CServerApp::CArgs::s_instance = NULL;

CServerApp::CServerApp()
{
}

CServerApp::~CServerApp()
{
}

CServerApp::CArgs::CArgs() :
m_pname(NULL),
m_backend(false),
m_restartable(true),
m_daemon(true),
m_configFile(),
m_logFilter(NULL),
m_logFile(NULL),
m_display(NULL),
m_synergyAddress(NULL),
m_config(NULL)
{
	s_instance = this; 
}

CServerApp::CArgs::~CArgs()
{
	s_instance = NULL;
}