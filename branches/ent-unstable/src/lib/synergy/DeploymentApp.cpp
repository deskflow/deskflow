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

#include "synergy/ArgsBase.h"
#include "synergy/DeploymentApp.h"
#include "zeroconf/Bonjour.h"

#include <iostream>

NS1(synergy)

enum {
	kErrorOk,
	kErrorArgs,
	kErrorException,
	kErrorUnknown
};

CDeploymentApp::CDeploymentApp() :
	CMinimalApp(new CArgsBase())
{
}

CDeploymentApp::~CDeploymentApp()
{
}

int
CDeploymentApp::run(int argc, char** argv)
{
	try {
		parseArgs(argc, argv);
		
		zeroconf::CBonjour bonjour;
		bonjour.registerService();

		// TODO: use signal wait
		while (true) {
			LOG((CLOG_DEBUG "waiting..."));
			ARCH->sleep(1);
		}
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
		return kErrorException;
	}
	catch (...) {
		std::cerr << "unknown error" << std::endl;
		return kErrorUnknown;
	}

	return kErrorOk;
}

_NS1
