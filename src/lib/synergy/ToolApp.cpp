/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2014 Synergy Si Ltd.
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
 */

#include "synergy/ToolApp.h"

#include "synergy/ArgParser.h"
#include "synergy/SubscriptionManager.h"
#include "arch/Arch.h"
#include "base/Log.h"
#include "base/String.h"

#include <iostream>
#include <sstream>

#if SYSAPI_WIN32
#include "platform/MSWindowsSession.h"
#endif

#define JSON_URL "https://synergy-project.org/premium/json/"

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
				String output = synergy::string::sprintf("activeDesktop:%s", name.c_str());
				LOG((CLOG_INFO "%s", output.c_str()));
			}
#endif
		}
		else if (m_args.m_loginAuthenticate) {
			loginAuth();
		}
		else if (m_args.m_getPluginList) {
			getPluginList();
		}
		else if (m_args.m_getInstalledDir) {
			std::cout << ARCH->getInstalledDirectory() << std::endl;
		}
		else if (m_args.m_getPluginDir) {
			std::cout << ARCH->getPluginDirectory() << std::endl;
		}
		else if (m_args.m_getProfileDir) {
			std::cout << ARCH->getProfileDirectory() << std::endl;
		}
		else if (m_args.m_getArch) {
			std::cout << ARCH->getPlatformName() << std::endl;
		}
		else if (!m_args.m_subscriptionSerial.empty()) {
			try {
				SubscriptionManager subscriptionManager;
				subscriptionManager.activate(m_args.m_subscriptionSerial);
			}
			catch (XSubscription& e) {
				LOG((CLOG_CRIT "subscription error: %s", e.what()));
				return kExitSubscription;
			}
		}
		else if (m_args.m_getSubscriptionFilename) {
			try {
				SubscriptionManager subscriptionManager;
				subscriptionManager.printFilename();
			}
			catch (XSubscription& e) {
				LOG((CLOG_CRIT "subscription error: %s", e.what()));
				return kExitSubscription;
			}
		}
		else if (m_args.m_checkSubscription) {
			try {
				SubscriptionManager subscriptionManager;
				subscriptionManager.checkFile("");
			}
			catch (XSubscription& e) {
				LOG((CLOG_CRIT "subscription error: %s", e.what()));
				return kExitSubscription;
			}
		}
		else if (m_args.m_notifyActivation) {
			notifyActivation();
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

#if WINAPI_XWINDOWS
	// HACK: avoid sigsegv on linux
	m_bye(kErrorOk);
#endif

	return kErrorOk;
}

void
ToolApp::help()
{
}

void
ToolApp::loginAuth()
{
	String credentials;
	std::cin >> credentials;

	size_t separator1 = credentials.find(':');
	size_t separator2 = credentials.find(':', separator1 + 1);
	size_t separator3 = credentials.find(':', separator2 + 1);
	String email = credentials.substr(0, separator1);
	String password = credentials.substr(separator1 + 1, separator2 - separator1 - 1);
	String macHash;
	String os;
	if (separator3 != String::npos) {
		macHash = credentials.substr(separator2 + 1, separator3 - separator2 - 1);
		os = credentials.substr(separator3 + 1, credentials.length() - separator3 - 1);
	}
	else {
		macHash = credentials.substr(separator2 + 1, credentials.length() - separator2 - 1);
		os = ARCH->getOSName();
	}

	std::stringstream ss;
	ss << JSON_URL << "auth/";
	ss << "?email=" << ARCH->internet().urlEncode(email);
	ss << "&password=" << password;
	ss << "&mac=" << macHash;
	ss << "&os=" << ARCH->internet().urlEncode(os);
	ss << "&arch=" << ARCH->internet().urlEncode(ARCH->getPlatformName());

	std::cout << ARCH->internet().get(ss.str()) << std::endl;
}

void
ToolApp::getPluginList()
{
	String credentials;
	std::cin >> credentials;

	size_t separator = credentials.find(':');
	String email = credentials.substr(0, separator);
	String password = credentials.substr(separator + 1, credentials.length());

	std::stringstream ss;
	ss <<  JSON_URL << "plugins/";
	ss << "?email=" << ARCH->internet().urlEncode(email);
	ss << "&password=" << password;

	std::cout << ARCH->internet().get(ss.str()) << std::endl;
}

void 
ToolApp::notifyActivation()
{
	String info;
	std::cin >> info;

	size_t separator = info.find(':');
	String action = info.substr(0, separator);
	String macHash = info.substr(separator + 1, info.length());

	std::stringstream ss;
	ss <<  JSON_URL << "notify/";
	ss << "?action=" << action;
	ss << "&mac=" << macHash;
	ss << "&os=" << ARCH->internet().urlEncode(ARCH->getOSName());
	ss << "&arch=" << ARCH->internet().urlEncode(ARCH->getPlatformName());

	std::cout << ARCH->internet().get(ss.str()) << std::endl;
}
