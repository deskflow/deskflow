/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Nick Bolton
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

#include "CDaemonApp.h"
#include <string>
#include <iostream>

#define ERROR_NONE 0
#define ERROR_ARG 1
#define ERROR_EX 2

using namespace std;

CDaemonApp::CDaemonApp()
{
}

CDaemonApp::~CDaemonApp()
{
}

int
CDaemonApp::run(int argc, char** argv)
{
	try {
		// if no args, daemonize.
		if (argc == 1) {
			daemonize();
			return ERROR_NONE;
		}
		else {
			for (int i = 1; i < argc; ++i) {

				string arg(argv[i]);

#if SYSAPI_WIN32
				if (arg == "--install") {
					installService();
					continue;
				}
				else if (arg == "--uninstall") {
					uninstallService();
					continue;
				}
#endif

				cerr << "unrecognized arg: " << arg << endl;
				return ERROR_ARG;
			}
		}
	}
	catch (std::exception& e) {
		cerr << e.what() << endl;
		return ERROR_EX;
	}
	catch (...) {
		cerr << "unknown exception" << endl;
		return ERROR_EX;
	}

	return ERROR_NONE;
}

void
CDaemonApp::daemonize()
{
	cout << "demonizing" << endl;
}

#if SYSAPI_WIN32

void
CDaemonApp::installService()
{
	cout << "install service" << endl;
}

void
CDaemonApp::uninstallService()
{
	cout << "uninstall service" << endl;
}

#endif
