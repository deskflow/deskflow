/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Synergy Si, inc.
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

#include "synergy/ArgParser.h"

#include "synergy/ArgsBase.h"
#include "base/Log.h"

CArgsBase* CArgParser::m_argsBase = NULL;

bool
CArgParser::parsePlatformArg(CArgsBase& argsBase, const int& argc, const char* const* argv, int& i)
{
#if WINAPI_MSWINDOWS
	if (isArg(i, argc, argv, NULL, "--service")) {
		LOG((CLOG_WARN "obsolete argument --service, use synergyd instead."));
		argsBase.m_shouldExit = true;
	}
	else if (isArg(i, argc, argv, NULL, "--exit-pause")) {
		argsBase.m_pauseOnExit = true;
	}
	else if (isArg(i, argc, argv, NULL, "--stop-on-desk-switch")) {
		argsBase.m_stopOnDeskSwitch = true;
	}
	else {
		// option not supported here
		return false;
	}

	return true;
#elif WINAPI_XWINDOWS
	if (CArgumentParser::isArg(i, argc, argv, "-display", "--display", 1)) {
		// use alternative display
		argsBase.m_display = argv[++i];
	}

	else if (CArgumentParser::isArg(i, argc, argv, NULL, "--no-xinitthreads")) {
		argsBase.m_disableXInitThreads = true;
	}

	else {
		// option not supported here
		return false;
	}

	return true;
#elif WINAPI_CARBON
	// no options for carbon
	return false;
#endif
}

bool
CArgParser::isArg(
	int argi, int argc, const char* const* argv,
	const char* name1, const char* name2,
	int minRequiredParameters)
{
	if ((name1 != NULL && strcmp(argv[argi], name1) == 0) ||
		(name2 != NULL && strcmp(argv[argi], name2) == 0)) {
			// match.  check args left.
			if (argi + minRequiredParameters >= argc) {
				LOG((CLOG_PRINT "%s: missing arguments for `%s'" BYE,
					argsBase().m_pname, argv[argi], argsBase().m_pname));
				argsBase().m_shouldExit = true;
				return false;
			}
			return true;
	}

	// no match
	return false;
}
