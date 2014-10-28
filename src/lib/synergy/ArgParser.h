/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Synergy Si, Inc.
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
 
#pragma once

#include "base/String.h"
#include "common/stdvector.h"

class CServerArgs;
class CClientArgs;
class CToolArgs;
class CArgsBase;
class CApp;

class CArgParser {

public:
	CArgParser(CApp* app);

	bool				parseServerArgs(CServerArgs& args, int argc, const char* const* argv);
	bool				parseClientArgs(CClientArgs& args, int argc, const char* const* argv);
	bool				parsePlatformArg(CArgsBase& argsBase, const int& argc, const char* const* argv, int& i);
	bool				parseToolArgs(CToolArgs& args, int argc, const char* const* argv);
	bool				parseGenericArgs(int argc, const char* const* argv, int& i);
	void				setArgsBase(CArgsBase& argsBase) { m_argsBase = &argsBase; }

	static	bool		isArg(int argi, int argc, const char* const* argv,
							const char* name1, const char* name2,
							int minRequiredParameters = 0);
	static void			splitCommandString(CString& command, std::vector<CString>& argv);
	static bool			searchDoubleQuotes(CString& command, size_t& left, 
							size_t& right, size_t startPos = 0);
	static void			removeDoubleQuotes(CString& arg);
	static const char**	getArgv(std::vector<CString>& argsArray);
	static CString		assembleCommand(std::vector<CString>& argsArray, 
							CString ignoreArg = "", int parametersRequired = 0);

private:
	void				updateCommonArgs(const char* const* argv);
	bool				checkUnexpectedArgs();
	
	static CArgsBase&	argsBase() { return *m_argsBase; }

private:
	CApp*				m_app;
	
	static CArgsBase*	m_argsBase;
};
