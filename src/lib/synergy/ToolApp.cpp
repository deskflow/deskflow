/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Synergy Si Ltd.
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

#include "synergy/ToolApp.h"

#include "synergy/ArgParser.h"
#include "arch/Arch.h"
#include "base/Log.h"
#include "base/String.h"

#include <iostream>
#include <sstream>

#if SYSAPI_WIN32
#include "platform/MSWindowsSession.h"
#endif

enum {
	kErrorOk,
	kErrorArgs,
	kErrorException,
	kErrorUnknown
};

UInt32
ToolApp::run(int argc, char** argv)
{
	if (argc <= 1) {
		std::cerr << "no args" << std::endl;
		return kErrorArgs;
	}

	try {
		ArgParser argParser(this);
		bool result = argParser.parseToolArgs(m_args, argc, argv);

		if (!result) {
			m_bye(kExitArgs);
		}

		if (m_args.m_printActiveDesktopName) {
#if SYSAPI_WIN32
			MSWindowsSession session;
			String name = session.getActiveDesktopName();
			if (name.empty()) {
				LOG((CLOG_CRIT "failed to get active desktop name"));
				return kExitFailed;
			}
			else {
				std::cout << "activeDesktop:" << name.c_str() << std::endl;
			}
#endif
		}
		else {
			throw XSynergy("Nothing to do");
		}
	}
	catch (std::exception& e) {
		LOG((CLOG_CRIT "An error occurred: %s\n", e.what()));
		return kExitFailed;
	}
	catch (...) {
		LOG((CLOG_CRIT "An unknown error occurred.\n"));
		return kExitFailed;
	}

	return kErrorOk;
}

void
ToolApp::help()
{
}
