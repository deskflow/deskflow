/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman, Nick Bolton, Sorin Sbarnea
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

#include "CApp.h"
#include "CLog.h"
#include "Version.h"
#include "ProtocolTypes.h"
#include "CArch.h"
#include "XBase.h"
#include "XArch.h"
#if SYSAPI_WIN32
#include "CArchMiscWindows.h"
#endif
#include "LogOutputters.h"
#include "XSynergy.h"

#include <iostream>
#include <stdio.h>

#if WINAPI_CARBON
#include <ApplicationServices/ApplicationServices.h>
#endif

CApp* CApp::s_instance = nullptr;

CApp::CApp(CreateTaskBarReceiverFunc createTaskBarReceiver, CArgsBase* args) :
m_createTaskBarReceiver(createTaskBarReceiver),
m_args(args),
m_bye(&exit),
m_taskBarReceiver(NULL),
m_suspended(false)
{
	assert(s_instance == nullptr);
	s_instance = this;
}

CApp::~CApp()
{
	delete m_args;
}

CApp::CArgsBase::CArgsBase() :
#if SYSAPI_WIN32
m_daemon(false), // daemon mode not supported on windows (use --service)
m_debugServiceWait(false),
m_relaunchMode(false),
m_pauseOnExit(false),
#if GAME_DEVICE_SUPPORT
m_gameDevice(false),
#endif
#else
m_daemon(true), // backward compatibility for unix (daemon by default)
#endif
#if WINAPI_XWINDOWS
m_disableXInitThreads(false),
#endif
m_backend(false),
m_restartable(true),
m_noHooks(false),
m_disableTray(false),
m_pname(NULL),
m_logFilter(NULL),
m_logFile(NULL),
m_display(NULL)
{
}

CApp::CArgsBase::~CArgsBase()
{
}

bool
CApp::isArg(
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
				m_bye(kExitArgs);
			}
			return true;
	}

	// no match
	return false;
}

bool
CApp::parseArg(const int& argc, const char* const* argv, int& i)
{
	if (ARCH->parseArg(argc, argv, i)) {
		// handled by platform util
		return true;
	}
	
	else if (isArg(i, argc, argv, "-d", "--debug", 1)) {
		// change logging level
		argsBase().m_logFilter = argv[++i];
	}

	else if (isArg(i, argc, argv, "-l", "--log", 1)) {
		argsBase().m_logFile = argv[++i];
	}

	else if (isArg(i, argc, argv, "-f", "--no-daemon")) {
		// not a daemon
		argsBase().m_daemon = false;
	}

	else if (isArg(i, argc, argv, NULL, "--daemon")) {
		// daemonize
		argsBase().m_daemon = true;
	}

	else if (isArg(i, argc, argv, "-n", "--name", 1)) {
		// save screen name
		argsBase().m_name = argv[++i];
	}

	else if (isArg(i, argc, argv, "-1", "--no-restart")) {
		// don't try to restart
		argsBase().m_restartable = false;
	}

	else if (isArg(i, argc, argv, NULL, "--restart")) {
		// try to restart
		argsBase().m_restartable = true;
	}

	else if (isArg(i, argc, argv, "-z", NULL)) {
		argsBase().m_backend = true;
	}

	else if (isArg(i, argc, argv, NULL, "--no-hooks")) {
		argsBase().m_noHooks = true;
	}

	else if (isArg(i, argc, argv, "-h", "--help")) {
		help();
		m_bye(kExitSuccess);
	}

	else if (isArg(i, argc, argv, NULL, "--version")) {
		version();
		m_bye(kExitSuccess);
	}
	
	else if (isArg(i, argc, argv, NULL, "--no-tray")) {
		argsBase().m_disableTray = true;
	}

	else {
		// option not supported here
		return false;
	}

	return true;
}

void
CApp::parseArgs(int argc, const char* const* argv, int& i)
{
	// about these use of assert() here:
	// previously an /analyze warning was displayed if we only used assert and
	// did not return on failure. however, this warning does not appear to show
	// any more (could be because new compiler args have been added).
	// the asserts are programmer benefit only; the os should never pass 0 args,
	// because the first is always the binary name. the only way assert would 
	// evaluate to true, is if this parse function were implemented incorrectly,
	// which is unlikely because it's old code and has a specific use.
	// we should avoid using anything other than assert here, because it will
	// look like important code, which it's not really.
	assert(argsBase().m_pname != NULL);
	assert(argv != NULL);
	assert(argc >= 1);

	// set defaults
	argsBase().m_name = ARCH->getHostName();

	// parse options
	for (i = 1; i < argc; ++i) {

		if (parseArg(argc, argv, i)) {
			continue;
		}

		else if (isArg(i, argc, argv, "--", NULL)) {
			// remaining arguments are not options
			++i;
			break;
		}

		else if (argv[i][0] == '-') {
			std::cerr << "Unrecognized option: " << argv[i] << std::endl;
			m_bye(kExitArgs);
		}

		else {
			// this and remaining arguments are not options
			break;
		}
	}

#if SYSAPI_WIN32
	// suggest that user installs as a windows service. when launched as 
	// service, process should automatically detect that it should run in
	// daemon mode.
	if (argsBase().m_daemon) {
		LOG((CLOG_ERR 
			"the --daemon argument is not supported on windows. "
			"instead, install %s as a service (--service install)", 
			argsBase().m_pname));
		m_bye(kExitArgs);
	}
#endif
}

void
CApp::version()
{
	char buffer[500];
	sprintf(
		buffer,
		"%s %s, protocol version %d.%d\n%s",
		argsBase().m_pname,
		kVersion,
		kProtocolMajorVersion,
		kProtocolMinorVersion,
		kCopyright
		);

	std::cout << buffer << std::endl;
}

int
CApp::run(int argc, char** argv)
{
#if SYSAPI_WIN32
	// record window instance for tray icon, etc
	CArchMiscWindows::setInstanceWin32(GetModuleHandle(NULL));
#endif

#if MAC_OS_X_VERSION_10_7
	// dock hide only supported on lion :(
	ProcessSerialNumber psn = { 0, kCurrentProcess };
	GetCurrentProcess(&psn);
	TransformProcessType(&psn, kProcessTransformToBackgroundApplication);
#endif

	CArch arch;

	// install application in to arch
	ARCH->adoptApp(this);

	// create an instance of log
	CLOG;
	
	// HACK: fail by default (saves us setting result in each catch)
	int result = kExitFailed;

	try {
		result = ARCH->run(argc, argv);
	}
	catch (XExitApp& e) {
		// instead of showing a nasty error, just exit with the error code.
		// not sure if i like this behaviour, but it's probably better than 
		// using the exit(int) function!
		result = e.getCode();
	}
	catch (XBase& e) {
		LOG((CLOG_CRIT "Exception: %s\n", e.what()));
	}
	catch (XArch& e) {
		LOG((CLOG_CRIT "Init failed: %s" BYE, e.what().c_str(), argsBase().m_pname));
	}
	catch (std::exception& e) {
		LOG((CLOG_CRIT "Exception: %s\n", e.what()));
	}
	catch (...) {
		LOG((CLOG_CRIT "An unexpected exception occurred.\n"));
	}

	delete CLOG;

	ARCH->beforeAppExit();
	
	return result;
}

int
CApp::daemonMainLoop(int, const char**)
{
#if SYSAPI_WIN32
	CSystemLogger sysLogger(daemonName(), false);
#else
	CSystemLogger sysLogger(daemonName(), true);
#endif
	return mainLoop();
}

void 
CApp::setupFileLogging()
{
	if (argsBase().m_logFile != NULL) {
		m_fileLog = new CFileLogOutputter(argsBase().m_logFile);
		CLOG->insert(m_fileLog);
		LOG((CLOG_DEBUG1 "logging to file (%s) enabled", argsBase().m_logFile));
	}
}

void 
CApp::loggingFilterWarning()
{
	if (CLOG->getFilter() > CLOG->getConsoleMaxLevel()) {
		if (argsBase().m_logFile == NULL) {
			LOG((CLOG_WARN "log messages above %s are NOT sent to console (use file logging)", 
				CLOG->getFilterName(CLOG->getConsoleMaxLevel())));
		}
	}
}

void 
CApp::initApp(int argc, const char** argv)
{
	// parse command line
	parseArgs(argc, argv);

	// setup file logging after parsing args
	setupFileLogging();

	// load configuration
	loadConfig();

	if (!argsBase().m_disableTray) {

		// create a log buffer so we can show the latest message
		// as a tray icon tooltip
		CBufferedLogOutputter* logBuffer = new CBufferedLogOutputter(1000);
		CLOG->insert(logBuffer, true);

		// make the task bar receiver.  the user can control this app
		// through the task bar.
		m_taskBarReceiver = m_createTaskBarReceiver(logBuffer);
	}
}
