#include "CClient.h"
#include "CPlatform.h"
#include "ProtocolTypes.h"
#include "Version.h"
#include "CNetwork.h"
#include "CNetworkAddress.h"
#include "XSocket.h"
#include "CCondVar.h"
#include "CLock.h"
#include "CMutex.h"
#include "CThread.h"
#include "XThread.h"
#include "CLog.h"
#include "CString.h"
#include <cstring>

// platform dependent name of a daemon
#if defined(CONFIG_PLATFORM_WIN32)
#define DAEMON "service"
#define DAEMON_NAME "Synergy Client"
#elif defined(CONFIG_PLATFORM_UNIX)
#define DAEMON "daemon"
#define DAEMON_NAME "synergy"
#endif

//
// program arguments
//

static const char*		pname         = NULL;
static bool				s_restartable = true;
static bool				s_daemon      = true;
static bool				s_camp        = true;
static bool				s_install     = false;
static bool				s_uninstall   = false;
static const char*		s_logFilter   = NULL;
static CString			s_name;
static CNetworkAddress	s_serverAddress;


//
// logging thread safety
//

static CMutex*			s_logMutex = NULL;

static
void
logLock(
	bool lock)
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
// platform independent main
//

static CClient*			s_client = NULL;

static
int
realMain(
	CMutex* mutex)
{
	try {
		// initialize threading library
		CThread::init();

		// make logging thread safe
		CMutex logMutex;
		s_logMutex = &logMutex;
		CLog::setLock(&logLock);

		bool locked = true;
		try {
			// create client
			s_client = new CClient(s_name);
			s_client->camp(s_camp);

			// run client
			if (mutex != NULL) {
				mutex->unlock();
			}
			locked = false;
			bool success = s_client->run(s_serverAddress);
			locked = true;
			if (mutex != NULL) {
				mutex->lock();
			}

			// clean up
			delete s_client;
			s_client = NULL;
			CNetwork::cleanup();
			CLog::setLock(NULL);
			s_logMutex = NULL;

			return success ? 16 : 0;
		}
		catch (...) {
			// clean up
			if (!locked && mutex != NULL) {
				mutex->lock();
			}
			delete s_client;
			s_client = NULL;
			CNetwork::cleanup();
			CLog::setLock(NULL);
			s_logMutex = NULL;
			throw;
		}
	}
	catch (XBase& e) {
		log((CLOG_CRIT "failed: %s", e.what()));
		return 16;
	}
	catch (XThread&) {
		// terminated
		return 1;
	}

	return 0;
}

static
int
restartMain()
{
	return realMain(NULL);
}

// invoke realMain and wait for it.  if s_restartable then keep
// restarting realMain until it returns a terminate code.
static
int
restartableMain()
{
	if (s_restartable) {
		CPlatform platform;
		return platform.restart(restartMain, 16);
	}
	else {
		return realMain(NULL);
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
	log((CLOG_PRINT
"%s %d.%d.%d, protocol version %d.%d\n"
"%s",
								pname,
								kMajorVersion,
								kMinorVersion,
								kReleaseVersion,
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
" [--"DAEMON"|--no-"DAEMON"]"
" [--camp|--no-camp]"
" [--debug <level>]"
" [--name <screen-name>]"
" [--restart|--no-restart]"
" [--install]"
" <server-address>\n"
"or\n"
" --uninstall\n"
"Start the synergy mouse/keyboard sharing server.\n"
"\n"
"*     --camp               keep attempting to connect to the server until\n"
"                           successful.\n"
"      --no-camp            do not camp.\n"
"  -d, --debug <level>      filter out log messages with priorty below level.\n"
"                           level may be: FATAL, ERROR, WARNING, NOTE, INFO,\n"
"                           DEBUG, DEBUG1, DEBUG2.\n"
"  -f, --no-"DAEMON"          run the client in the foreground.\n"
"*     --"DAEMON"             run the client as a "DAEMON".\n"
"  -n, --name <screen-name> use screen-name instead the hostname to identify\n"
"                           ourself to the server.\n"
"  -1, --no-restart         do not try to restart the client if it fails for\n"
"                           some reason.\n"
"*     --restart            restart the client automatically if it fails for\n"
"                           some unexpected reason, including the server\n"
"                           disconnecting but not including failing to\n"
"                           connect to the server."
"      --install            install server as a "DAEMON".\n"
"      --uninstall          uninstall server "DAEMON".\n"
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
"client is running as a "DAEMON".",
								pname, kDefaultPort));

}

static
bool
isArg(int argi,
	int argc,
	const char** argv,
	const char* name1,
	const char* name2,
	int minRequiredParameters = 0)
{
	if ((name1 != NULL && strcmp(argv[argi], name1) == 0) ||
		(name2 != NULL && strcmp(argv[argi], name2) == 0)) {
		// match.  check args left.
		if (argi + minRequiredParameters >= argc) {
			log((CLOG_PRINT "%s: missing arguments for `%s'" BYE,
								pname, argv[argi], pname));
			bye(2);
		}
		return true;
	}

	// no match
	return false;
}

static
void
parse(
	int argc,
	const char** argv)
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

		else if (isArg(i, argc, argv, "-f", "--no-"DAEMON)) {
			// not a daemon
			s_daemon = false;
		}

		else if (isArg(i, argc, argv, NULL, "--"DAEMON)) {
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

		else if (isArg(i, argc, argv, "-h", "--help")) {
			help();
			bye(0);
		}

		else if (isArg(i, argc, argv, NULL, "--version")) {
			version();
			bye(0);
		}

		else if (isArg(i, argc, argv, NULL, "--install")) {
#if !defined(CONFIG_PLATFORM_WIN32)
			log((CLOG_PRINT "%s: `%s' not supported on this platform" BYE,
								pname, argv[i], pname));
			bye(2);
#endif
			s_install = true;
			if (s_uninstall) {
				log((CLOG_PRINT "%s: `--install' and `--uninstall'"
								" are mutually exclusive" BYE,
								pname, argv[i], pname));
				bye(2);
			}
		}

		else if (isArg(i, argc, argv, NULL, "--uninstall")) {
#if !defined(CONFIG_PLATFORM_WIN32)
			log((CLOG_PRINT "%s: `%s' not supported on this platform" BYE,
								pname, argv[i], pname));
			bye(2);
#endif
			s_uninstall = true;
			if (s_install) {
				log((CLOG_PRINT "%s: `--install' and `--uninstall'"
								" are mutually exclusive" BYE,
								pname, argv[i], pname));
				bye(2);
			}
		}

		else if (isArg(i, argc, argv, "--", NULL)) {
			// remaining arguments are not options
			++i;
			break;
		}

		else if (argv[i][0] == '-') {
			log((CLOG_PRINT "%s: unrecognized option `%s'" BYE,
								pname, argv[i], pname));
			bye(2);
		}

		else {
			// this and remaining arguments are not options
			break;
		}
	}

	// exactly one non-option argument (server-address) unless using
	// --uninstall.
	if (s_uninstall) {
		if (i != argc) {
			log((CLOG_PRINT "%s: unrecognized option `%s' to `%s'" BYE,
								pname, argv[i], pname,
								s_install ? "--install" : "--uninstall"));
			bye(2);
		}
	}
	else {
		if (i == argc) {
			log((CLOG_PRINT "%s: a server address or name is required" BYE,
								pname, pname));
			bye(1);
		}
		if (i + 1 != argc) {
			log((CLOG_PRINT "%s: unrecognized option `%s'" BYE,
								pname, argv[i], pname));
			bye(2);
		}

		// save server address
		try {
			s_serverAddress = CNetworkAddress(argv[i], kDefaultPort);
		}
		catch (XSocketAddress&) {
			log((CLOG_PRINT "%s: invalid server address" BYE,
								pname, pname));
			bye(2);
		}
	}

	// increase default filter level for daemon.  the user must
	// explicitly request another level for a daemon.
	if (s_daemon && s_logFilter == NULL) {
#if defined(CONFIG_PLATFORM_WIN32)
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
		bye(2);
	}
}


//
// platform dependent entry points
//

#if defined(CONFIG_PLATFORM_WIN32)

#include "CMSWindowsScreen.h"

static
bool
logMessageBox(
	int priority,
	const char* msg)
{
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
	s_client->quit();
}

static
int
daemonStartup(
	IPlatform* iplatform,
	int argc,
	const char** argv)
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
		throw CWin32Platform::CDaemonFailed(1);
	}

	// run as a service
	return platform->runDaemon(realMain, daemonStop);
}

static
int
daemonStartup95(
	IPlatform*,
	int,
	const char**)
{
	return realMain(NULL);
}

static
bool
logDiscard(
	int,
	const char*)
{
	return true;
}

static bool				s_die = false;

static
void
checkParse(int e)
{
	// anything over 1 means invalid args.  1 means missing args.
	// 0 means graceful exit.  we plan to exit for anything but
	// 1 (missing args);  the service control manager may supply
	// the missing arguments so we don't exit in that case.
	s_die = (e != 1);
	throw s_die;
}

int WINAPI
WinMain(
	HINSTANCE instance,
	HINSTANCE,
	LPSTR,
	int)
{
	CPlatform platform;

	// save instance
	CMSWindowsScreen::init(instance);

	// get program name
	pname = platform.getBasename(__argv[0]);

	// initialize network library
	CNetwork::init();

	// parse command line without reporting errors but recording if
	// the app would've exited.  this is too avoid showing a dialog
	// box if we're being started as a service because we shouldn't
	// take too long to startup in that case.  this mostly works but
	// will choke if the service control manager passes --install
	// or --uninstall (but that's unlikely).
	CLog::setOutputter(&logDiscard);
	bye = &checkParse;
	try {
		parse(__argc, const_cast<const char**>(__argv));
	}
	catch (...) {
		// ignore
	}

	// if we're not starting as an NT service then reparse the command
	// line normally.
	if (s_die || !s_daemon || s_install || s_uninstall ||
		CWin32Platform::isWindows95Family()) {
		// send PRINT and FATAL output to a message box
		CLog::setOutputter(&logMessageBox);

		// exit on bye
		bye = &exit;

		// reparse
		parse(__argc, const_cast<const char**>(__argv));
	}

	// if starting as a daemon then we ignore the startup command line
	// here.  we'll parse the command line passed in when the service
	// control manager calls us back.
	else {
		// do nothing
	}

	// install/uninstall
	if (s_install) {
		// get the full path to this program
		TCHAR path[MAX_PATH];
		if (GetModuleFileName(NULL, path,
								sizeof(path) / sizeof(path[0])) == 0) {
			log((CLOG_CRIT "cannot determine absolute path to program"));
			return 16;
		}

		// construct the command line to start the service with
		CString commandLine = "--"DAEMON;
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
		commandLine += " ";
		commandLine += s_serverAddress.getHostname().c_str();

		// install
		if (!platform.installDaemon(DAEMON_NAME,
					"Shares this system's mouse and keyboard with others.",
					path, commandLine.c_str())) {
			log((CLOG_CRIT "failed to install service"));
			return 16;
		}
		log((CLOG_PRINT "installed successfully"));
		return 0;
	}
	else if (s_uninstall) {
		if (!platform.uninstallDaemon(DAEMON_NAME)) {
			log((CLOG_CRIT "failed to uninstall service"));
			return 16;
		}
		log((CLOG_PRINT "uninstalled successfully"));
		return 0;
	}

	// daemonize if requested
	int result;
	if (s_daemon) {
		if (CWin32Platform::isWindows95Family()) {
			result = platform.daemonize(DAEMON_NAME, &daemonStartup95);
		}
		else {
			result = platform.daemonize(DAEMON_NAME, &daemonStartup);
		}
		if (result == -1) {
			log((CLOG_CRIT "failed to start as a service"));
			return 16;
		}
	}
	else {
		result = restartableMain();
	}

	return result;
}

#elif defined(CONFIG_PLATFORM_UNIX)

static
int
daemonStartup(
	IPlatform*,
	int,
	const char**)
{
	return restartableMain();
}

int
main(
	int argc,
	char** argv)
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
			return 16;
		}
	}
	else {
		result = restartableMain();
	}

	return result;
}

#else

#error no main() for platform

#endif
