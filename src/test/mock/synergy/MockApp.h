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

#define TEST_ENV

#include "synergy/App.h"

#include "test/global/gmock.h"

class MockApp : public App
{
public:
	MockApp() : App(NULL, NULL, NULL) { }

	MOCK_METHOD0(help, void());
	MOCK_METHOD0(loadConfig, void());
	MOCK_METHOD1(loadConfig, bool(const String&));
	MOCK_CONST_METHOD0(daemonInfo, const char*());
	MOCK_CONST_METHOD0(daemonName, const char*());
	MOCK_METHOD2(parseArgs, void(int, const char* const*));
	MOCK_METHOD0(version, void());
	MOCK_METHOD2(standardStartup, int(int, char**));
	MOCK_METHOD4(runInner, int(int, char**, ILogOutputter*, StartupFunc));
	MOCK_METHOD0(startNode, void());
	MOCK_METHOD0(mainLoop, int());
	MOCK_METHOD2(foregroundStartup, int(int, char**));
	MOCK_METHOD0(createScreen, synergy::Screen*());
};
