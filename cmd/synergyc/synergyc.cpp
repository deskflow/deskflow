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

#include "CClient.h"
#include "ISecondaryScreenFactory.h"
#include "CPlatform.h"
#include "ProtocolTypes.h"
#include "Version.h"
#include "XScreen.h"
#include "CNetwork.h"
#include "CNetworkAddress.h"
#include "CTCPSocketFactory.h"
#include "XSocket.h"
#include "CCondVar.h"
#include "CLock.h"
#include "CMutex.h"
#include "CThread.h"
#include "XThread.h"
#include "CLog.h"
#include "CString.h"
#include <cstring>

#if WINDOWS_LIKE
#include "CMSWindowsSecondaryScreen.h"
#include "resource.h"
#elif UNIX_LIKE
#include "CXWindowsSecondaryScreen.h"
#endif

// platform dependent name of a daemon
#if WINDOWS_LIKE
#define DAEMON_NAME "Synergy Client"
#elif UNIX_LIKE
#define DAEMON_NAME "synergyc"
#endif

//
// program arguments
//

static const char*		pname         = NULL;
static bool				s_backend     = false;
static bool				s_restartable = true;
static bool				s_daemon      = true;
static bool				s_camp        = true;
static const char*		s_logFilter   = NULL;
static CString			s_name;
static CNetworkAddress	s_serverAddress;


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

class CSecondaryScreenFactory : public ISecondaryScreenFactory {
public:
	CSecondaryScreenFactory() { }
	virtual ~CSecondaryScreenFactory() { }

	// ISecondaryScreenFactory overrides
	virtual CSecondaryScreen*
						create(IScreenReceiver*);
};

CSecondaryScreen*
CSecondaryScreenFactory::create(IScreenReceiver* receiver)
{
#if WINDOWS_LIKE
	return new CMSWindowsSecondaryScreen(receiver);
#elif UNIX_LIKE
	return new CXWindowsSecondaryScreen(receiver);
#endif
}


//
// platform independent main
//

static CClient*			s_client = NULL;

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
			// create client
			s_client = new CClient(s_name);
			s_client->camp(s_camp);
			s_client->setAddress(s_serverAddress);
			s_client->setScreenFactory(new CSecondaryScreenFactory);
			s_client->setSocketFactory(new CTCPSocketFactory);
			s_client->setStreamFilterFactory(NULL);

			// open client
			try {
				s_client->open();
				opened = true;

				// run client
				if (mutex != NULL) {
					mutex->unlock();
				}
				locked = false;
				s_client->mainLoop();
				locked = true;

				// get client status
				if (s_client->wasRejected()) {
					// try again later.  we don't want to bother
					// the server very often if it doesn't want us.
					throw XScreenUnavailable(60.0);
				}

				// clean up
#define FINALLY do {								\
				if (!locked && mutex != NULL) {		\
					mutex->lock();					\
				}									\
				if (s_client != NULL) {				\
					if (opened) {					\
						s_client->close();			\
					}								\
				}									\
				delete s_client;					\
				s_client = NULL;					\
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
	log((CLOG_PRINT
"Usage: %s"
" [--camp|--no-camp]"
" [--daemon|--no-daemon]"
" [--debug <level>]"
" [--name <screen-name>]"
" [--restart|--no-restart]"
" <server-address>\n"
"\n"
"Start the synergy mouse/keyboard sharing server.\n"
"\n"
"*     --camp               keep attempting to connect to the server until\n"
"                           successful.\n"
"      --no-camp            do not camp.\n"
"  -d, --debug <level>      filter out log messages with priorty below level.\n"
"                           level may be: FATAL, ERROR, WARNING, NOTE, INFO,\n"
"                           DEBUG, DEBUG1, DEBUG2.\n"
"  -f, --no-daemon          run the client in the foreground.\n"
"*     --daemon             run the client as a daemon.\n"
"  -n, --name <screen-name> use screen-name instead the hostname to identify\n"
"                           ourself to the server.\n"
"  -1, --no-restart         do not try to restart the client if it fails for\n"
"                           some reason.\n"
"*     --restart            restart the client automatically if it fails.\n"
"  -h, --help               display this help and exit.\n"
"      --version            display version information and exit.\n"
"\n"
"* marks defaults.\n"
"\n"
"The server address is of the form: [<hostname>][:<port>].  The hostname\n"
"must be the address or hostname of the server.  The port overrides the\n"
"default port, %d.\n"
"\n"
"Where log messages go depends on the platform and whether or not the\n"
"client is running as a daemon.",
								pname, kDefaultPort));

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

		else if (isArg(i, argc, argv, "-n", "--name", 1)) {
			// save screen name
			s_name = argv[++i];
		}

		else if (isArg(i, argc, argv, NULL, "--camp")) {
			// enable camping
			s_camp = true;
		}

		else if (isArg(i, argc, argv, NULL, "--no-camp")) {
			// disable camping
			s_camp = false;
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

	// exactly one non-option argument (server-address)
	if (i == argc) {
		log((CLOG_PRINT "%s: a server address or name is required" BYE,
								pname, pname));
		bye(kExitArgs);
	}
	if (i + 1 != argc) {
		log((CLOG_PRINT "%s: unrecognized option `%s'" BYE,
								pname, argv[i], pname));
		bye(kExitArgs);
	}

	// save server address
	try {
		s_serverAddress = CNetworkAddress(argv[i], kDefaultPort);
	}
	catch (XSocketAddress& e) {
		log((CLOG_PRINT "%s: %s" BYE,
								pname, e.what(), pname));
		bye(kExitFailed);
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

//
// platform dependent entry points
//

#if WINDOWS_LIKE

#include "CMSWindowsScreen.h"

static
bool
logMessageBox(int priority, const char* msg)
{
	if (priority <= (s_backend ? CLog::kERROR : CLog::kFATAL)) {
		MessageBox(NULL, msg, pname, MB_OK | MB_ICONWARNING);
		return true;
	}
	else {
		return s_backend;
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
	s_client->exitMainLoop();
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
	parse(argc, argv);

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
