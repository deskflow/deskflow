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
#include "CPlatform.h"
#include "ProtocolTypes.h"
#include "Version.h"
#include "XScreen.h"
#include "CNetwork.h"
#include "CTCPSocketFactory.h"
#include "XSocket.h"
#include "CLock.h"
#include "CMutex.h"
#include "CThread.h"
#include "XThread.h"
#include "CLog.h"
#include "stdfstream.h"
#include <cstring>

#if WINDOWS_LIKE
#include "CMSWindowsPrimaryScreen.h"
#include "resource.h"
#elif UNIX_LIKE
#include "CXWindowsPrimaryScreen.h"
#endif

// platform dependent name of a daemon
#if WINDOWS_LIKE
#define DAEMON_NAME "Synergy Server"
#elif UNIX_LIKE
#define DAEMON_NAME "synergyd"
#endif

// configuration file name
#if WINDOWS_LIKE
#define CONFIG_NAME "synergy.sgc"
#elif UNIX_LIKE
#define CONFIG_NAME "synergy.conf"
#endif

//
// program arguments
//

static const char*		pname         = NULL;
static bool				s_backend     = false;
static bool				s_restartable = true;
static bool				s_daemon      = true;
#if WINDOWS_LIKE
static bool				s_install     = false;
static bool				s_uninstall   = false;
#endif
static const char*		s_configFile  = NULL;
static const char*		s_logFilter   = NULL;
static CString			s_name;
static CNetworkAddress	s_synergyAddress;
static CNetworkAddress	s_httpAddress;
static CConfig			s_config;


//
// logging thread safety
//

static CMutex*			s_logMutex = NULL;

static
void
logLock(bool lock)
{
	assert(s_logMutex != NULL);

	if (lock) {
		s_logMutex->lock();
	}
	else {
		s_logMutex->unlock();
	}
}


//
// platform dependent factories
//

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


//
// platform independent main
//

static CServer*			s_server = NULL;

static
int
realMain(CMutex* mutex)
{
	// caller should have mutex locked on entry

	// initialize threading library
	CThread::init();

	// make logging thread safe
	CMutex logMutex;
	s_logMutex = &logMutex;
	CLog::setLock(&logLock);

	int result = kExitSuccess;
	do {
		bool opened = false;
		bool locked = true;
		try {
			// if configuration has no screens then add this system
			// as the default
			if (s_config.begin() == s_config.end()) {
				s_config.addScreen(s_name);
			}

			// set the contact address, if provided, in the config.
			// otherwise, if the config doesn't have an address, use
			// the default.
			if (s_synergyAddress.isValid()) {
				s_config.setSynergyAddress(s_synergyAddress);
			}
			else if (!s_config.getSynergyAddress().isValid()) {
				s_config.setSynergyAddress(CNetworkAddress(kDefaultPort));
			}

			// set HTTP address if provided
			if (s_httpAddress.isValid()) {
				s_config.setHTTPAddress(s_httpAddress);
			}

			// create server
			s_server = new CServer(s_name);
			s_server->setConfig(s_config);
			s_server->setScreenFactory(new CPrimaryScreenFactory);
			s_server->setSocketFactory(new CTCPSocketFactory);
			s_server->setStreamFilterFactory(NULL);

			// open server
			try {
				s_server->open();
				opened = true;

				// run server (unlocked)
				if (mutex != NULL) {
					mutex->unlock();
				}
				locked = false;
				s_server->mainLoop();
				locked = true;

				// clean up
#define FINALLY do {								\
				if (!locked && mutex != NULL) {		\
					mutex->lock();					\
				}									\
				if (s_server != NULL) {				\
					if (opened) {					\
						s_server->close();			\
					}								\
				}									\
				delete s_server;					\
				s_server = NULL;					\
				} while (false)
				FINALLY;
			}
			catch (XScreenUnavailable& e) {
				// wait before retrying if we're going to retry
				if (s_restartable) {
					CThread::sleep(e.getRetryTime());
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
				s_restartable = false;
				result        = kExitFailed;
				FINALLY;
			}
#undef FINALLY
		}
		catch (XBase& e) {
			log((CLOG_CRIT "failed: %s", e.what()));
		}
		catch (XThread&) {
			// terminated
			s_restartable = false;
			result        = kExitTerminated;
		}
		catch (...) {
			CLog::setLock(NULL);
			s_logMutex = NULL;
			throw;
		}
	} while (s_restartable);

	// clean up
	CLog::setLock(NULL);
	s_logMutex = NULL;

	return result;
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
	log((CLOG_PRINT
"%s %s, protocol version %d.%d\n"
"%s",
								pname,
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
" {--daemon|--no-daemon}"											\
" [--install]\n"													\
"or\n"																\
" --uninstall\n"
#  define PLATFORM_DESC												\
"      --install            install server as a daemon.\n"			\
"      --uninstall          uninstall server daemon.\n"
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

	CPlatform platform;

	log((CLOG_PRINT
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
"following to load sets the configuration:\n"
"  %s\n"
"  %s\n"
"If no configuration file can be loaded then the configuration uses its\n"
"defaults with just the server screen.\n"
"\n"
"Where log messages go depends on the platform and whether or not the\n"
"server is running as a daemon.",
								pname,
								kDefaultPort,
								platform.addPathComponent(
									platform.getUserDirectory(),
									CONFIG_NAME).c_str(),
								platform.addPathComponent(
									platform.getSystemDirectory(),
									CONFIG_NAME).c_str()));
}

static
bool
isArg(int argi, int argc, const char** argv,
				const char* name1, const char* name2,
				int minRequiredParameters = 0)
{
	if ((name1 != NULL && strcmp(argv[argi], name1) == 0) ||
		(name2 != NULL && strcmp(argv[argi], name2) == 0)) {
		// match.  check args left.
		if (argi + minRequiredParameters >= argc) {
			log((CLOG_PRINT "%s: missing arguments for `%s'" BYE,
								pname, argv[argi], pname));
			bye(kExitArgs);
		}
		return true;
	}

	// no match
	return false;
}

static
void
parse(int argc, const char** argv)
{
	assert(pname != NULL);
	assert(argv  != NULL);
	assert(argc  >= 1);

	// set defaults
	char hostname[256];
	if (CNetwork::gethostname(hostname, sizeof(hostname)) != CNetwork::Error) {
		s_name = hostname;
	}

	// parse options
	int i;
	for (i = 1; i < argc; ++i) {
		if (isArg(i, argc, argv, "-d", "--debug", 1)) {
			// change logging level
			s_logFilter = argv[++i];
		}

		else if (isArg(i, argc, argv, "-a", "--address", 1)) {
			// save listen address
			try {
				s_synergyAddress = CNetworkAddress(argv[i + 1], kDefaultPort);
			}
			catch (XSocketAddress& e) {
				log((CLOG_PRINT "%s: %s" BYE,
								pname, e.what(), pname));
				bye(kExitArgs);
			}
			++i;
		}

		else if (isArg(i, argc, argv, NULL, "--http", 1)) {
			// save listen address
			try {
				s_httpAddress = CNetworkAddress(argv[i + 1], kDefaultPort + 1);
			}
			catch (XSocketAddress& e) {
				log((CLOG_PRINT "%s: %s" BYE,
								pname, e.what(), pname));
				bye(kExitArgs);
			}
			++i;
		}

		else if (isArg(i, argc, argv, "-n", "--name", 1)) {
			// save screen name
			s_name = argv[++i];
		}

		else if (isArg(i, argc, argv, "-c", "--config", 1)) {
			// save configuration file path
			s_configFile = argv[++i];
		}

		else if (isArg(i, argc, argv, "-f", "--no-daemon")) {
			// not a daemon
			s_daemon = false;
		}

		else if (isArg(i, argc, argv, NULL, "--daemon")) {
			// daemonize
			s_daemon = true;
		}

		else if (isArg(i, argc, argv, "-1", "--no-restart")) {
			// don't try to restart
			s_restartable = false;
		}

		else if (isArg(i, argc, argv, NULL, "--restart")) {
			// try to restart
			s_restartable = true;
		}

		else if (isArg(i, argc, argv, "-z", NULL)) {
			s_backend = true;
		}

		else if (isArg(i, argc, argv, "-h", "--help")) {
			help();
			bye(kExitSuccess);
		}

		else if (isArg(i, argc, argv, NULL, "--version")) {
			version();
			bye(kExitSuccess);
		}

#if WINDOWS_LIKE
		else if (isArg(i, argc, argv, NULL, "--install")) {
			s_install = true;
			if (s_uninstall) {
				log((CLOG_PRINT "%s: `--install' and `--uninstall'"
								" are mutually exclusive" BYE,
								pname, argv[i], pname));
				bye(kExitArgs);
			}
		}
#endif

#if WINDOWS_LIKE
		else if (isArg(i, argc, argv, NULL, "--uninstall")) {
			s_uninstall = true;
			if (s_install) {
				log((CLOG_PRINT "%s: `--install' and `--uninstall'"
								" are mutually exclusive" BYE,
								pname, argv[i], pname));
				bye(kExitArgs);
			}
		}
#endif

		else if (isArg(i, argc, argv, "--", NULL)) {
			// remaining arguments are not options
			++i;
			break;
		}

		else if (argv[i][0] == '-') {
			log((CLOG_PRINT "%s: unrecognized option `%s'" BYE,
								pname, argv[i], pname));
			bye(kExitArgs);
		}

		else {
			// this and remaining arguments are not options
			break;
		}
	}

	// no non-option arguments are allowed
	if (i != argc) {
		log((CLOG_PRINT "%s: unrecognized option `%s'" BYE,
								pname, argv[i], pname));
		bye(kExitArgs);
	}

	// increase default filter level for daemon.  the user must
	// explicitly request another level for a daemon.
	if (s_daemon && s_logFilter == NULL) {
#if WINDOWS_LIKE
		if (CPlatform::isWindows95Family()) {
			// windows 95 has no place for logging so avoid showing
			// the log console window.
			s_logFilter = "FATAL";
		}
		else
#endif
		{
			s_logFilter = "NOTE";
		}
	}

	// set log filter
	if (!CLog::setFilter(s_logFilter)) {
		log((CLOG_PRINT "%s: unrecognized log level `%s'" BYE,
								pname, s_logFilter, pname));
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
		log((CLOG_DEBUG "opening configuration \"%s\"", pathname));
		std::ifstream configStream(pathname);
		if (!configStream) {
			throw XConfigRead("cannot open configuration");
		}
		configStream >> s_config;
		log((CLOG_DEBUG "configuration read successfully"));
		return true;
	}
	catch (XConfigRead& e) {
		if (require) {
			log((CLOG_PRINT "%s: cannot read configuration '%s': %s",
								pname, pathname, e.what()));
			bye(kExitConfig);
		}
		else {
			log((CLOG_DEBUG "cannot read configuration \"%s\": %s",
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
	if (s_configFile != NULL) {
		// require the user specified file to load correctly
		loadConfig(s_configFile, true);
	}

	// load the default configuration if no explicit file given
	else {
		// get the user's home directory.  use the effective user id
		// so a user can't get a setuid root program to load his file.
		CPlatform platform;
		bool loaded = false;
		CString path = platform.getUserDirectory();
		if (!path.empty()) {
			// complete path
			path = platform.addPathComponent(path, CONFIG_NAME);

			// now try loading the user's configuration
			loaded = loadConfig(path.c_str(), false);
		}
		if (!loaded) {
			// try the system-wide config file
			path = platform.getSystemDirectory();
			if (!path.empty()) {
				path = platform.addPathComponent(path, CONFIG_NAME);
				loadConfig(path.c_str(), false);
			}
		}
	}
}

//
// platform dependent entry points
//

#if WINDOWS_LIKE

#include "CMSWindowsScreen.h"

static bool				s_errors = false;

static
bool
logMessageBox(int priority, const char* msg)
{
	if (priority <= CLog::kERROR) {
		s_errors = true;
	}
	if (s_backend) {
		return true;
	}
	if (priority <= CLog::kFATAL) {
		MessageBox(NULL, msg, pname, MB_OK | MB_ICONWARNING);
		return true;
	}
	else {
		return false;
	}
}

static
void
byeThrow(int x)
{
	throw CWin32Platform::CDaemonFailed(x);
}

static
void
daemonStop(void)
{
	s_server->exitMainLoop();
}

static
int
daemonStartup(IPlatform* iplatform, int argc, const char** argv)
{
	// get platform pointer
	CWin32Platform* platform = static_cast<CWin32Platform*>(iplatform);

	// catch errors that would normally exit
	bye = &byeThrow;

	// parse command line
	s_install   = false;
	s_uninstall = false;
	parse(argc, argv);
	if (s_install || s_uninstall) {
		// not allowed to install/uninstall from service
		throw CWin32Platform::CDaemonFailed(kExitArgs);
	}

	// load configuration
	loadConfig();

	// run as a service
	return platform->runDaemon(realMain, daemonStop);
}

static
int
daemonStartup95(IPlatform*, int, const char**)
{
	return realMain(NULL);
}

int WINAPI
WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int)
{
	CPlatform platform;

	// save instance
	CMSWindowsScreen::init(instance);

	// get program name
	pname = platform.getBasename(__argv[0]);

	// initialize network library
	CNetwork::init();

	// send PRINT and FATAL output to a message box
	CLog::setOutputter(&logMessageBox);

	// windows NT family starts services using no command line options.
	// since i'm not sure how to tell the difference between that and
	// a user providing no options we'll assume that if there are no
	// arguments and we're on NT then we're being invoked as a service.
	// users on NT can use `--daemon' or `--no-daemon' to force us out
	// of the service code path.
	if (__argc <= 1 && !CWin32Platform::isWindows95Family()) {
		int result = platform.daemonize(DAEMON_NAME, &daemonStartup);
		if (result == -1) {
			log((CLOG_CRIT "failed to start as a service" BYE, pname));
			return kExitFailed;
		}
		return result;
	}

	// parse command line
	parse(__argc, const_cast<const char**>(__argv));

	// install/uninstall
	if (s_install) {
		// get the full path to this program
		TCHAR path[MAX_PATH];
		if (GetModuleFileName(NULL, path,
								sizeof(path) / sizeof(path[0])) == 0) {
			log((CLOG_CRIT "cannot determine absolute path to program"));
			return kExitFailed;
		}

		// construct the command line to start the service with
		CString commandLine;
		commandLine += "--daemon";
		if (s_restartable) {
			commandLine += " --restart";
		}
		else {
			commandLine += " --no-restart";
		}
		if (s_logFilter != NULL) {
			commandLine += " --debug ";
			commandLine += s_logFilter;
		}
		if (s_configFile != NULL) {
			commandLine += " --config \"";
			commandLine += s_configFile;
			commandLine += "\"";
		}

		// install
		if (!platform.installDaemon(DAEMON_NAME,
					"Shares this system's mouse and keyboard with others.",
					path, commandLine.c_str())) {
			log((CLOG_CRIT "failed to install service"));
			return kExitFailed;
		}
		log((CLOG_PRINT "installed successfully"));
		return kExitSuccess;
	}
	else if (s_uninstall) {
		switch (platform.uninstallDaemon(DAEMON_NAME)) {
		case IPlatform::kSuccess:
			log((CLOG_PRINT "uninstalled successfully"));
			return kExitSuccess;

		case IPlatform::kFailed:
			log((CLOG_CRIT "failed to uninstall service"));
			return kExitFailed;

		case IPlatform::kAlready:
			log((CLOG_CRIT "service isn't installed"));
			// return success since service is uninstalled
			return kExitSuccess;
		}
	}

	// load configuration
	loadConfig();

	// daemonize if requested
	int result;
	if (s_daemon) {
		// redirect log messages
		platform.installDaemonLogger(DAEMON_NAME);

		// start as a daemon
		if (CWin32Platform::isWindows95Family()) {
			result = platform.daemonize(DAEMON_NAME, &daemonStartup95);
			if (result == -1) {
				log((CLOG_CRIT "failed to start as a service" BYE, pname));
				result = kExitFailed;
			}
		}
		else {
			// cannot start a service from the command line so just
			// run normally (except with log messages redirected).
			result = realMain(NULL);
		}
	}
	else {
		// run
		result = realMain(NULL);
	}

	CNetwork::cleanup();

	// if running as a non-daemon backend and there was an error then
	// wait for the user to click okay so he can see the error messages.
	if (s_backend && !s_daemon && (result == kExitFailed || s_errors)) {
		char msg[1024];
		msg[0] = '\0';
		LoadString(instance, IDS_FAILED, msg, sizeof(msg) / sizeof(msg[0]));
		MessageBox(NULL, msg, pname, MB_OK | MB_ICONWARNING);
	}

	return result;
}

#elif UNIX_LIKE

static
int
daemonStartup(IPlatform*, int, const char**)
{
	return realMain(NULL);
}

int
main(int argc, char** argv)
{
	CPlatform platform;

	// get program name
	pname = platform.getBasename(argv[0]);

	// initialize network library
	CNetwork::init();

	// parse command line
	parse(argc, const_cast<const char**>(argv));

	// load configuration
	loadConfig();

	// daemonize if requested
	int result;
	if (s_daemon) {
		result = platform.daemonize(DAEMON_NAME, &daemonStartup);
		if (result == -1) {
			log((CLOG_CRIT "failed to daemonize"));
			return kExitFailed;
		}
	}
	else {
		result = realMain(NULL);
	}

	CNetwork::cleanup();

	return result;
}

#else

#error no main() for platform

#endif
