/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file COPYING that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "CServer.h"
#include "CConfig.h"
#include "IPrimaryScreenFactory.h"
#include "ProtocolTypes.h"
#include "Version.h"
#include "XScreen.h"
#include "CTCPSocketFactory.h"
#include "XSocket.h"
#include "CLock.h"
#include "CMutex.h"
#include "CThread.h"
#include "XThread.h"
#include "CFunctionJob.h"
#include "CLog.h"
#include "LogOutputters.h"
#include "CArch.h"
#include "stdfstream.h"
#include <cstring>

#define DAEMON_RUNNING(running_)
#if WINDOWS_LIKE
#include "CMSWindowsScreen.h"
#include "CMSWindowsPrimaryScreen.h"
#include "CArchMiscWindows.h"
#include "CMSWindowsServerTaskBarReceiver.h"
#include "resource.h"
#undef DAEMON_RUNNING
#define DAEMON_RUNNING(running_) CArchMiscWindows::daemonRunning(running_)
#elif UNIX_LIKE
#include "CXWindowsPrimaryScreen.h"
#include "CXWindowsServerTaskBarReceiver.h"
#endif

// platform dependent name of a daemon
#if WINDOWS_LIKE
#define DAEMON_NAME "Synergy Server"
#elif UNIX_LIKE
#define DAEMON_NAME "synergys"
#endif

// configuration file name
#if WINDOWS_LIKE
#define USR_CONFIG_NAME "synergy.sgc"
#define SYS_CONFIG_NAME "synergy.sgc"
#elif UNIX_LIKE
#define USR_CONFIG_NAME ".synergy.conf"
#define SYS_CONFIG_NAME "synergy.conf"
#endif

//
// program arguments
//

#define ARG CArgs::s_instance

class CArgs {
public:
	CArgs() :
		m_pname(NULL),
		m_backend(false),
		m_restartable(true),
		m_daemon(true),
		m_configFile(NULL),
		m_logFilter(NULL)
		{ s_instance = this; }
	~CArgs() { s_instance = NULL; }

public:
	static CArgs*		s_instance;
	const char* 		m_pname;
	bool				m_backend;
	bool				m_restartable;
	bool				m_daemon;
	const char* 		m_configFile;
	const char* 		m_logFilter;
	CString 			m_name;
	CNetworkAddress 	m_synergyAddress;
	CNetworkAddress 	m_httpAddress;
	CConfig 			m_config;
};

CArgs*					CArgs::s_instance = NULL;


//
// platform dependent factories
//

//! Factory for creating primary screens
/*!
Objects of this type create primary screens appropriate for the
platform.
*/
class CPrimaryScreenFactory : public IPrimaryScreenFactory {
public:
	CPrimaryScreenFactory() { }
	virtual ~CPrimaryScreenFactory() { }

	// IPrimaryScreenFactory overrides
	virtual CPrimaryScreen*
						create(IScreenReceiver*, IPrimaryScreenReceiver*);
};

CPrimaryScreen*
CPrimaryScreenFactory::create(IScreenReceiver* receiver,
				IPrimaryScreenReceiver* primaryReceiver)
{
#if WINDOWS_LIKE
	return new CMSWindowsPrimaryScreen(receiver, primaryReceiver);
#elif UNIX_LIKE
	return new CXWindowsPrimaryScreen(receiver, primaryReceiver);
#endif
}


//! CQuitJob
/*!
A job that cancels a given thread.
*/
class CQuitJob : public IJob {
public:
	CQuitJob(const CThread& thread);
	~CQuitJob();

	// IJob overrides
	virtual void		run();

private:
	CThread				m_thread;
};

CQuitJob::CQuitJob(const CThread& thread) :
	m_thread(thread)
{
	// do nothing
}

CQuitJob::~CQuitJob()
{
	// do nothing
}

void
CQuitJob::run()
{
	m_thread.cancel();
}


//
// platform independent main
//

static CServer*					s_server          = NULL;
static CServerTaskBarReceiver*	s_taskBarReceiver = NULL;

static
int
realMain(void)
{
	int result = kExitSuccess;
	do {
		bool opened = false;
		bool locked = true;
		try {
			// if configuration has no screens then add this system
			// as the default
			if (ARG->m_config.begin() == ARG->m_config.end()) {
				ARG->m_config.addScreen(ARG->m_name);
			}

			// set the contact address, if provided, in the config.
			// otherwise, if the config doesn't have an address, use
			// the default.
			if (ARG->m_synergyAddress.isValid()) {
				ARG->m_config.setSynergyAddress(ARG->m_synergyAddress);
			}
			else if (!ARG->m_config.getSynergyAddress().isValid()) {
				ARG->m_config.setSynergyAddress(CNetworkAddress(kDefaultPort));
			}

			// set HTTP address if provided
			if (ARG->m_httpAddress.isValid()) {
				ARG->m_config.setHTTPAddress(ARG->m_httpAddress);
			}

			// create server
			s_server = new CServer(ARG->m_name);
			s_server->setConfig(ARG->m_config);
			s_server->setScreenFactory(new CPrimaryScreenFactory);
			s_server->setSocketFactory(new CTCPSocketFactory);
			s_server->setStreamFilterFactory(NULL);

			// open server
			try {
				s_taskBarReceiver->setServer(s_server);
				s_server->open();
				opened = true;

				// run server
				DAEMON_RUNNING(true);
				locked = false;
				s_server->mainLoop();

				// clean up
#define FINALLY do {									\
					if (!locked) {						\
						DAEMON_RUNNING(false);			\
						locked = true;					\
					}									\
					if (opened) {						\
						s_server->close();				\
					}									\
					s_taskBarReceiver->setServer(NULL);	\
					delete s_server;					\
					s_server = NULL;					\
				} while (false)
				FINALLY;
			}
			catch (XScreenUnavailable& e) {
				// wait before retrying if we're going to retry
				if (ARG->m_restartable) {
					ARCH->sleep(e.getRetryTime());
				}
				else {
					result = kExitFailed;
				}
				FINALLY;
			}
			catch (XThread&) {
				FINALLY;
				throw;
			}
			catch (...) {
				// don't try to restart and fail
				ARG->m_restartable = false;
				result             = kExitFailed;
				FINALLY;
			}
#undef FINALLY
		}
		catch (XBase& e) {
			LOG((CLOG_CRIT "failed: %s", e.what()));
		}
		catch (XThread&) {
			// terminated
			ARG->m_restartable = false;
			result             = kExitTerminated;
		}
	} while (ARG->m_restartable);

	return result;
}

static
void
realMainEntry(void* vresult)
{
	*reinterpret_cast<int*>(vresult) = realMain();
}

static
int
runMainInThread(void)
{
	int result = 0;
	CThread appThread(new CFunctionJob(&realMainEntry, &result));
	try {
#if WINDOWS_LIKE
		MSG msg;
		while (appThread.waitForEvent(-1.0) == CThread::kEvent) {
			// check for a quit event
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				if (msg.message == WM_QUIT) {
					CThread::getCurrentThread().cancel();
				}
			}
		}
#else
		appThread.wait(-1.0);
#endif
		return result;
	}
	catch (XThread&) {
		appThread.cancel();
		appThread.wait(-1.0);
		throw;
	}
}


//
// command line parsing
//

#define BYE "\nTry `%s --help' for more information."

static void				(*bye)(int) = &exit;

static
void
version()
{
	LOG((CLOG_PRINT
"%s %s, protocol version %d.%d\n"
"%s",
								ARG->m_pname,
								kVersion,
								kProtocolMajorVersion,
								kProtocolMinorVersion,
								kCopyright));
}

static
void
help()
{
#if WINDOWS_LIKE

#  define PLATFORM_ARGS												\
" {--daemon|--no-daemon}"
#  define PLATFORM_DESC
#  define PLATFORM_EXTRA											\
"At least one command line argument is required.  If you don't otherwise\n"	\
"need an argument use `--daemon'.\n"										\
"\n"

#else

#  define PLATFORM_ARGS												\
" [--daemon|--no-daemon]"
#  define PLATFORM_DESC
#  define PLATFORM_EXTRA

#endif

	LOG((CLOG_PRINT
"Usage: %s"
" [--address <address>]"
" [--config <pathname>]"
" [--debug <level>]"
" [--name <screen-name>]"
" [--restart|--no-restart]\n"
PLATFORM_ARGS
"\n"
"Start the synergy mouse/keyboard sharing server.\n"
"\n"
"  -a, --address <address>  listen for clients on the given address.\n"
"  -c, --config <pathname>  use the named configuration file instead.\n"
"  -d, --debug <level>      filter out log messages with priorty below level.\n"
"                           level may be: FATAL, ERROR, WARNING, NOTE, INFO,\n"
"                           DEBUG, DEBUG1, DEBUG2.\n"
"  -f, --no-daemon          run the server in the foreground.\n"
"*     --daemon             run the server as a daemon.\n"
"  -n, --name <screen-name> use screen-name instead the hostname to identify\n"
"                           this screen in the configuration.\n"
"  -1, --no-restart         do not try to restart the server if it fails for\n"
"                           some reason.\n"
"*     --restart            restart the server automatically if it fails.\n"
PLATFORM_DESC
"  -h, --help               display this help and exit.\n"
"      --version            display version information and exit.\n"
"\n"
"* marks defaults.\n"
"\n"
PLATFORM_EXTRA
"The argument for --address is of the form: [<hostname>][:<port>].  The\n"
"hostname must be the address or hostname of an interface on the system.\n"
"The default is to listen on all interfaces.  The port overrides the\n"
"default port, %d.\n"
"\n"
"If no configuration file pathname is provided then the first of the\n"
"following to load successfully sets the configuration:\n"
"  %s\n"
"  %s\n"
"If no configuration file can be loaded then the configuration uses its\n"
"defaults with just the server screen.\n"
"\n"
"Where log messages go depends on the platform and whether or not the\n"
"server is running as a daemon.",
								ARG->m_pname,
								kDefaultPort,
								ARCH->concatPath(
									ARCH->getUserDirectory(),
									USR_CONFIG_NAME).c_str(),
								ARCH->concatPath(
									ARCH->getSystemDirectory(),
									SYS_CONFIG_NAME).c_str()));
}

static
bool
isArg(int argi, int argc, const char* const* argv,
				const char* name1, const char* name2,
				int minRequiredParameters = 0)
{
	if ((name1 != NULL && strcmp(argv[argi], name1) == 0) ||
		(name2 != NULL && strcmp(argv[argi], name2) == 0)) {
		// match.  check args left.
		if (argi + minRequiredParameters >= argc) {
			LOG((CLOG_PRINT "%s: missing arguments for `%s'" BYE,
								ARG->m_pname, argv[argi], ARG->m_pname));
			bye(kExitArgs);
		}
		return true;
	}

	// no match
	return false;
}

static
void
parse(int argc, const char* const* argv)
{
	assert(ARG->m_pname != NULL);
	assert(argv       != NULL);
	assert(argc       >= 1);

	// set defaults
	ARG->m_name = ARCH->getHostName();

	// parse options
	int i;
	for (i = 1; i < argc; ++i) {
		if (isArg(i, argc, argv, "-d", "--debug", 1)) {
			// change logging level
			ARG->m_logFilter = argv[++i];
		}

		else if (isArg(i, argc, argv, "-a", "--address", 1)) {
			// save listen address
			try {
				ARG->m_synergyAddress = CNetworkAddress(argv[i + 1],
														kDefaultPort);
			}
			catch (XSocketAddress& e) {
				LOG((CLOG_PRINT "%s: %s" BYE,
								ARG->m_pname, e.what(), ARG->m_pname));
				bye(kExitArgs);
			}
			++i;
		}

		else if (isArg(i, argc, argv, NULL, "--http", 1)) {
			// save listen address
			try {
				ARG->m_httpAddress = CNetworkAddress(argv[i + 1],
														kDefaultPort + 1);
			}
			catch (XSocketAddress& e) {
				LOG((CLOG_PRINT "%s: %s" BYE,
								ARG->m_pname, e.what(), ARG->m_pname));
				bye(kExitArgs);
			}
			++i;
		}

		else if (isArg(i, argc, argv, "-n", "--name", 1)) {
			// save screen name
			ARG->m_name = argv[++i];
		}

		else if (isArg(i, argc, argv, "-c", "--config", 1)) {
			// save configuration file path
			ARG->m_configFile = argv[++i];
		}

		else if (isArg(i, argc, argv, "-f", "--no-daemon")) {
			// not a daemon
			ARG->m_daemon = false;
		}

		else if (isArg(i, argc, argv, NULL, "--daemon")) {
			// daemonize
			ARG->m_daemon = true;
		}

		else if (isArg(i, argc, argv, "-1", "--no-restart")) {
			// don't try to restart
			ARG->m_restartable = false;
		}

		else if (isArg(i, argc, argv, NULL, "--restart")) {
			// try to restart
			ARG->m_restartable = true;
		}

		else if (isArg(i, argc, argv, "-z", NULL)) {
			ARG->m_backend = true;
		}

		else if (isArg(i, argc, argv, "-h", "--help")) {
			help();
			bye(kExitSuccess);
		}

		else if (isArg(i, argc, argv, NULL, "--version")) {
			version();
			bye(kExitSuccess);
		}

		else if (isArg(i, argc, argv, "--", NULL)) {
			// remaining arguments are not options
			++i;
			break;
		}

		else if (argv[i][0] == '-') {
			LOG((CLOG_PRINT "%s: unrecognized option `%s'" BYE,
								ARG->m_pname, argv[i], ARG->m_pname));
			bye(kExitArgs);
		}

		else {
			// this and remaining arguments are not options
			break;
		}
	}

	// no non-option arguments are allowed
	if (i != argc) {
		LOG((CLOG_PRINT "%s: unrecognized option `%s'" BYE,
								ARG->m_pname, argv[i], ARG->m_pname));
		bye(kExitArgs);
	}

	// increase default filter level for daemon.  the user must
	// explicitly request another level for a daemon.
	if (ARG->m_daemon && ARG->m_logFilter == NULL) {
#if WINDOWS_LIKE
		if (CArchMiscWindows::isWindows95Family()) {
			// windows 95 has no place for logging so avoid showing
			// the log console window.
			ARG->m_logFilter = "FATAL";
		}
		else
#endif
		{
			ARG->m_logFilter = "NOTE";
		}
	}

	// set log filter
	if (!CLOG->setFilter(ARG->m_logFilter)) {
		LOG((CLOG_PRINT "%s: unrecognized log level `%s'" BYE,
								ARG->m_pname, ARG->m_logFilter, ARG->m_pname));
		bye(kExitArgs);
	}
}

static
bool
loadConfig(const char* pathname, bool require)
{
	assert(pathname != NULL);

	try {
		// load configuration
		LOG((CLOG_DEBUG "opening configuration \"%s\"", pathname));
		std::ifstream configStream(pathname);
		if (!configStream) {
			throw XConfigRead("cannot open file");
		}
		configStream >> ARG->m_config;
		LOG((CLOG_DEBUG "configuration read successfully"));
		return true;
	}
	catch (XConfigRead& e) {
		if (require) {
			LOG((CLOG_PRINT "%s: cannot read configuration '%s': %s",
								ARG->m_pname, pathname, e.what()));
			bye(kExitConfig);
		}
		else {
			LOG((CLOG_DEBUG "cannot read configuration \"%s\": %s",
								pathname, e.what()));
		}
	}
	return false;
}

static
void
loadConfig()
{
	// load the config file, if specified
	if (ARG->m_configFile != NULL) {
		// require the user specified file to load correctly
		loadConfig(ARG->m_configFile, true);
	}

	// load the default configuration if no explicit file given
	else {
		// get the user's home directory.  use the effective user id
		// so a user can't get a setuid root program to load his file.
		bool loaded = false;
		CString path = ARCH->getUserDirectory();
		if (!path.empty()) {
			// complete path
			path = ARCH->concatPath(path, USR_CONFIG_NAME);

			// now try loading the user's configuration
			loaded = loadConfig(path.c_str(), false);
		}
		if (!loaded) {
			// try the system-wide config file
			path = ARCH->getSystemDirectory();
			if (!path.empty()) {
				path = ARCH->concatPath(path, SYS_CONFIG_NAME);
				loadConfig(path.c_str(), false);
			}
		}
	}
}


//
// platform dependent entry points
//

#if WINDOWS_LIKE

static bool				s_hasImportantLogMessages = false;

//
// CMessageBoxOutputter
//
// This class writes severe log messages to a message box
//

class CMessageBoxOutputter : public ILogOutputter {
public:
	CMessageBoxOutputter() { }
	virtual ~CMessageBoxOutputter() { }

	// ILogOutputter overrides
	virtual void		open(const char*) { }
	virtual void		close() { }
	virtual bool		write(ELevel level, const char* message);
	virtual const char*	getNewline() const { return ""; }
};

bool
CMessageBoxOutputter::write(ELevel level, const char* message)
{
	// note any important messages the user may need to know about
	if (level <= CLog::kWARNING) {
		s_hasImportantLogMessages = true;
	}

	// FATAL and PRINT messages get a dialog box if not running as
	// backend.  if we're running as a backend the user will have
	// a chance to see the messages when we exit.
	if (!ARG->m_backend && level <= CLog::kFATAL) {
		MessageBox(NULL, message, ARG->m_pname, MB_OK | MB_ICONWARNING);
		return false;
	}
	else {
		return true;
	}
}

static
void
byeThrow(int x)
{
	CArchMiscWindows::daemonFailed(x);
}

static
int
daemonStartup(int argc, const char** argv)
{
	CSystemLogger sysLogger(DAEMON_NAME);

	// have to cancel this thread to quit
	s_taskBarReceiver->setQuitJob(new CQuitJob(CThread::getCurrentThread()));

	// catch errors that would normally exit
	bye = &byeThrow;

	// parse command line
	parse(argc, argv);

	// cannot run as backend if running as a service
	ARG->m_backend = false;

	// load configuration
	loadConfig();

	// run as a service
	return CArchMiscWindows::runDaemon(realMain);
}

static
int
daemonStartup95(int, const char**)
{
	CSystemLogger sysLogger(DAEMON_NAME);
	return runMainInThread();
}

static
int
run(int argc, char** argv)
{
	// windows NT family starts services using no command line options.
	// since i'm not sure how to tell the difference between that and
	// a user providing no options we'll assume that if there are no
	// arguments and we're on NT then we're being invoked as a service.
	// users on NT can use `--daemon' or `--no-daemon' to force us out
	// of the service code path.
	if (argc <= 1 && !CArchMiscWindows::isWindows95Family()) {
		try {
			return ARCH->daemonize(DAEMON_NAME, &daemonStartup);
		}
		catch (XArchDaemon& e) {
			LOG((CLOG_CRIT "failed to start as a service: %s" BYE, e.what().c_str(), ARG->m_pname));
		}
		return kExitFailed;
	}

	// parse command line
	parse(argc, argv);

	// load configuration
	loadConfig();

	// daemonize if requested
	if (ARG->m_daemon) {
		// start as a daemon
		if (CArchMiscWindows::isWindows95Family()) {
			try {
				return ARCH->daemonize(DAEMON_NAME, &daemonStartup95);
			}
			catch (XArchDaemon& e) {
				LOG((CLOG_CRIT "failed to start as a service: %s" BYE, e.what().c_str(), ARG->m_pname));
			}
			return kExitFailed;
		}
		else {
			// cannot start a service from the command line so just
			// run normally (except with log messages redirected).
			CSystemLogger sysLogger(DAEMON_NAME);
			return runMainInThread();
		}
	}
	else {
		// run
		return runMainInThread();
	}
}

int WINAPI
WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int)
{
	CArch arch(instance);
	CLOG;
	CArgs args;

	// save instance
	CMSWindowsScreen::init(instance);

	// get program name
	ARG->m_pname = ARCH->getBasename(__argv[0]);

	// send PRINT and FATAL output to a message box
	CLOG->insert(new CMessageBoxOutputter);

	// save log messages
	CBufferedLogOutputter logBuffer(1000);
	CLOG->insert(&logBuffer, true);

	// make the task bar receiver.  the user can control this app
	// through the task bar.
	s_taskBarReceiver = new CMSWindowsServerTaskBarReceiver(instance,
															&logBuffer);
	s_taskBarReceiver->setQuitJob(new CQuitJob(CThread::getCurrentThread()));

	int result;
	try {
		// run in foreground or as a daemon
		result = run(__argc, __argv);
	}
	catch (...) {
		// note that we don't rethrow thread cancellation.  we'll
		// be exiting soon so it doesn't matter.  what we'd like
		// is for everything after this try/catch to be in a
		// finally block.
		result = kExitFailed;
	}

	// done with task bar receiver
	delete s_taskBarReceiver;

	// done with log buffer
	CLOG->remove(&logBuffer);

	// let user examine any messages if we're running as a backend
	// by putting up a dialog box before exiting.
	if (ARG->m_backend && s_hasImportantLogMessages) {
		char msg[1024];
		msg[0] = '\0';
		LoadString(instance, IDS_FAILED, msg, sizeof(msg) / sizeof(msg[0]));
		MessageBox(NULL, msg, ARG->m_pname, MB_OK | MB_ICONWARNING);
	}

	delete CLOG;
	return result;
}

#elif UNIX_LIKE

static
int
daemonStartup(int, const char**)
{
	CSystemLogger sysLogger(DAEMON_NAME);
	return realMain();
}

int
main(int argc, char** argv)
{
	CArch arch;
	CLOG;
	CArgs args;

	// get program name
	ARG->m_pname = ARCH->getBasename(argv[0]);

	// make the task bar receiver.  the user can control this app
	// through the task bar.
	s_taskBarReceiver = new CXWindowsServerTaskBarReceiver;
	s_taskBarReceiver->setQuitJob(new CQuitJob(CThread::getCurrentThread()));

	// parse command line
	parse(argc, argv);

	// load configuration
	loadConfig();

	// daemonize if requested
	int result;
	if (ARG->m_daemon) {
		try {
			result = ARCH->daemonize(DAEMON_NAME, &daemonStartup);
		}
		catch (XArchDaemon&) {
			LOG((CLOG_CRIT "failed to daemonize"));
			result = kExitFailed;
		}
	}
	else {
		result = realMain();
	}

	// done with task bar receiver
	delete s_taskBarReceiver;

	return result;
}

#else

#error no main() for platform

#endif
