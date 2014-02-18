/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Bolton Software Ltd.
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

#include "CToolApp.h"
#include "CString.h"
#include "CArch.h"

#include <iostream>
#include <sstream>
	
//#define PREMIUM_AUTH_URL "http://localhost/synergy/premium/json/auth/"
#define PREMIUM_AUTH_URL "https://synergy-foss.org/premium/json/auth/"

int
CToolApp::run(int argc, char** argv)
{
	if (argc <= 1) {
		std::cerr << "no args" << std::endl;
		return 1;
	}

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--premium-auth") == 0) {
			CString credentials;
			std::cin >> credentials;

			size_t separator = credentials.find(':');
			CString email = credentials.substr(0, separator);
			CString password = credentials.substr(separator + 1, credentials.length());
			
			std::stringstream ss;
			ss << PREMIUM_AUTH_URL;
			ss << "?email=" << email;
			ss << "&password=" << password;

			std::cout << ARCH->internet().get(ss.str()) << std::endl;
			return 0;
		}
		else {
			std::cerr << "unknown arg: " << argv[i] << std::endl;
			return 1;
		}
	}

	return 0;
}
