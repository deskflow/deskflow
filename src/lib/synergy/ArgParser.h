/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Synergy Si, Inc.
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
 * 
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the OpenSSL
 * library.
 * You must obey the GNU General Public License in all respects for all of
 * the code used other than OpenSSL. If you modify file(s) with this
 * exception, you may extend this exception to your version of the file(s),
 * but you are not obligated to do so. If you do not wish to do so, delete
 * this exception statement from your version. If you delete this exception
 * statement from all source files in the program, then also delete it here.
 */
 
#pragma once

#include "base/String.h"
#include "common/stdvector.h"

class ServerArgs;
class ClientArgs;
class ToolArgs;
class ArgsBase;
class App;

class ArgParser {

public:
	ArgParser(App* app);

	bool				parseServerArgs(ServerArgs& args, int argc, const char* const* argv);
	bool				parseClientArgs(ClientArgs& args, int argc, const char* const* argv);
	bool				parsePlatformArg(ArgsBase& argsBase, const int& argc, const char* const* argv, int& i);
	bool				parseToolArgs(ToolArgs& args, int argc, const char* const* argv);
	bool				parseGenericArgs(int argc, const char* const* argv, int& i);
	bool				parseDeprecatedArgs(int argc, const char* const* argv, int& i);
	void				setArgsBase(ArgsBase& argsBase) { m_argsBase = &argsBase; }

	static	bool		isArg(int argi, int argc, const char* const* argv,
							const char* name1, const char* name2,
							int minRequiredParameters = 0);
	static void			splitCommandString(String& command, std::vector<String>& argv);
	static bool			searchDoubleQuotes(String& command, size_t& left, 
							size_t& right, size_t startPos = 0);
	static void			removeDoubleQuotes(String& arg);
	static const char**	getArgv(std::vector<String>& argsArray);
	static String		assembleCommand(std::vector<String>& argsArray, 
							String ignoreArg = "", int parametersRequired = 0);

private:
	void				updateCommonArgs(const char* const* argv);
	bool				checkUnexpectedArgs();
	
	static ArgsBase&	argsBase() { return *m_argsBase; }

private:
	App*				m_app;
	
	static ArgsBase*	m_argsBase;
};
