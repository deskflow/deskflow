#include "CServer.h"
#include "CConfig.h"
#include "CLog.h"
#include "CLock.h"
#include "CMutex.h"
#include "CNetwork.h"
#include "CPlatform.h"
#include "CThread.h"
#include "XThread.h"
#include "ProtocolTypes.h"
#include "Version.h"
#include "stdfstream.h"
#include <assert.h>

// platform dependent name of a daemon
#if defined(CONFIG_PLATFORM_WIN32)
#define DAEMON "service"
#define DAEMON_NAME "Synergy Server"
#elif defined(CONFIG_PLATFORM_UNIX)
#define DAEMON "daemon"
#define DAEMON_NAME "synergyd"
#endif

// configuration file name
#if defined(CONFIG_PLATFORM_WIN32)
#define CONFIG_NAME "synergy.sgc"
#elif defined(CONFIG_PLATFORM_UNIX)
#define CONFIG_NAME "synergy.conf"
#endif

//
// program arguments
//

static const char*		pname         = NULL;
static bool				s_restartable = true;
static bool				s_daemon      = true;
static bool				s_install     = false;
static bool				s_uninstall   = false;
static const char*		s_configFile  = NULL;
static const char*		s_logFilter   = NULL;
static CConfig			s_config;


//
// logging thread safety
//

static CMutex*			s_logMutex = NULL;

static void				logLock(bool lock)
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

static CServer*			s_server = NULL;

static int				realMain(CMutex* mutex)
{
	// s_serverLock should have mutex locked on entry

	try {
		// initialize threading library
		CThread::init();

		// make logging thread safe
		CMutex logMutex;
		s_logMutex = &logMutex;
		CLog::setLock(&logLock);

		bool locked = true;
		try {
			// initialize network library
			CNetwork::init();

			// if configuration has no screens then add this system
			// as the default
			if (s_config.begin() == s_config.end()) {
				s_config.addScreen("primary");
			}

			// create server
			s_server = new CServer();

			// run server (unlocked)
			if (mutex != NULL) {
				mutex->unlock();
			}
			locked = false;
			s_server->setConfig(s_config);
			s_server->run();
			locked = true;
			if (mutex != NULL) {
				mutex->lock();
			}

			// clean up
			delete s_server;
			s_server = NULL;
			CNetwork::cleanup();
			CLog::setLock(NULL);
			s_logMutex = NULL;
		}
		catch (...) {
			// clean up
			if (!locked && mutex != NULL) {
				mutex->lock();
			}
			delete s_server;
			s_server = NULL;
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

static int				restartMain()
{
	return realMain(NULL);
}

// invoke realMain and wait for it.  if s_restartable then keep
// restarting realMain until it returns a terminate code.
static int				restartableMain()
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

static void				version()
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

static void				help()
{
	CPlatform platform;

	log((CLOG_PRINT
"Usage: %s"
" [--config <pathname>]"
" [--debug <level>]"
" [--"DAEMON"|--no-"DAEMON"]"
" [--restart|--no-restart]\n"
"or\n"
" --install\n"
" --uninstall\n"
"\n"
"Start the synergy mouse/keyboard sharing server.\n"
"\n"
"  -c, --config <pathname>  use the named configuration file instead\n"
"                           where ~ represents the user's home directory.\n"
"  -d, --debug <level>      filter out log messages with priorty below level.\n"
"                           level may be: FATAL, ERROR, WARNING, NOTE, INFO,\n"
"                           DEBUG, DEBUG1, DEBUG2.\n"
"  -f, --no-"DAEMON"          run the server in the foreground.\n"
"*     --"DAEMON"             run the server as a "DAEMON".\n"
"  -1, --no-restart         do not try to restart the server if it fails for\n"
"                           some reason.\n"
"*     --restart            restart the server automatically if it fails.\n"
"      --install            install server as a "DAEMON".\n"
"      --uninstall          uninstall server "DAEMON".\n"
"  -h, --help               display this help and exit.\n"
"      --version            display version information and exit.\n"
"\n"
"* marks defaults.\n"
"\n"
"If no configuration file pathname is provided then the first of the\n"
"following to load sets the configuration:\n"
"  %s\n"
"  %s\n"
"If no configuration file can be loaded then the configuration uses its\n"
"defaults with just the server screen.\n"
"\n"
"Where log messages go depends on the platform and whether or not the\n"
"server is running as a "DAEMON".",
								pname,
								platform.addPathComponent(
									platform.getUserDirectory(),
									CONFIG_NAME).c_str(),
								platform.addPathComponent(
									platform.getSystemDirectory(),
									CONFIG_NAME).c_str()));
}

static bool				isArg(int argi,
								int argc, const char** argv,
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

static void				parse(int argc, const char** argv)
{
	assert(pname != NULL);
	assert(argv  != NULL);
	assert(argc  >= 1);

	// parse options
	int i;
	for (i = 1; i < argc; ++i) {
		if (isArg(i, argc, argv, "-d", "--debug", 1)) {
			// change logging level
			s_logFilter = argv[++i];
		}

		else if (isArg(i, argc, argv, "-c", "--config", 1)) {
			// save configuration file path
			s_configFile = argv[++i];
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
			log((CLOG_PRINT "%s: `%s' not permitted on this platform" BYE,
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
			log((CLOG_PRINT "%s: `%s' not permitted on this platform" BYE,
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

	// no non-option arguments are allowed
	if (i != argc) {
		log((CLOG_PRINT "%s: unrecognized option `%s'" BYE,
								pname, argv[i], pname));
		bye(2);
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

static bool				loadConfig(const char* pathname, bool require)
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
			bye(3);
		}
		else {
			log((CLOG_DEBUG "cannot read configuration \"%s\": %s",
								pathname, e.what()));
		}
	}
	return false;
}

static void				loadConfig()
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

#if defined(CONFIG_PLATFORM_WIN32)

#include "CMSWindowsScreen.h"

static bool				logMessageBox(int priority, const char* msg)
{
	if (priority <= CLog::kFATAL) {
		MessageBox(NULL, msg, pname, MB_OK | MB_ICONWARNING);
		return true;
	}
	else {
		return false;
	}
}

static void				byeThrow(int x)
{
	throw CWin32Platform::CDaemonFailed(x);
}

static void				daemonStop(void)
{
	s_server->quit();
}

static int				daemonStartup(IPlatform* iplatform,
								int argc, const char** argv)
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

	// load configuration
	loadConfig();

	// run as a service
	return platform->runDaemon(realMain, daemonStop);
}

static int				daemonStartup95(IPlatform*, int, const char**)
{
	return realMain(NULL);
}

static bool				logDiscard(int, const char*)
{
	return true;
}

static bool				s_die = false;

static void				checkParse(int e)
{
	// anything over 1 means invalid args.  1 means missing args.
	// 0 means graceful exit.  we plan to exit for anything but
	// 1 (missing args);  the service control manager may supply
	// the missing arguments so we don't exit in that case.
	s_die = (e != 1);
	throw s_die;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int)
{
	CPlatform platform;

	// save instance
	CMSWindowsScreen::init(instance);

	// get program name
	pname = platform.getBasename(__argv[0]);

	// parse command line without reporting errors but recording if
	// the app would've exited.  this is too avoid showing a dialog
	// box if we're being started as a service because we shouldn't
	// take to long to startup as a service.  this mostly works but
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
		CString commandLine;
		commandLine += "--"DAEMON;
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

	// load configuration
	loadConfig();

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

static int				daemonStartup(IPlatform*, int, const char**)
{
	return restartableMain();
}

int main(int argc, char** argv)
{
	CPlatform platform;

	// get program name
	pname = platform.getBasename(argv[0]);

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
