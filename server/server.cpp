#include "CServer.h"
#include "CConfig.h"
#include "CLog.h"
#include "CMutex.h"
#include "CNetwork.h"
#include "CPlatform.h"
#include "CThread.h"
#include "XThread.h"
#include "ProtocolTypes.h"
#include "Version.h"
#include "stdfstream.h"
#include <assert.h>

// configuration file name
#if defined(CONFIG_PLATFORM_WIN32)
#define CONFIG_NAME "synergy.sgc"
#define CONFIG_USER_DIR "%HOME%/"
#define CONFIG_SYS_DIR ""
#elif defined(CONFIG_PLATFORM_UNIX)
#define CONFIG_NAME "synergy.conf"
#define CONFIG_USER_DIR "~/"
#define CONFIG_SYS_DIR "/etc/"
#endif

//
// program arguments
//

static const char*		pname         = NULL;
static bool				s_restartable = true;
static bool				s_daemon      = true;
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
// main
//

void					realMain()
{
	// initialize threading library
	CThread::init();

	// make logging thread safe
	CMutex logMutex;
	s_logMutex = &logMutex;
	CLog::setLock(&logLock);

	CServer* server = NULL;
	try {
		// initialize network library
		CNetwork::init();

		// if configuration has no screens then add this system
		// as the default
		if (s_config.begin() == s_config.end()) {
			s_config.addScreen("primary");
		}

		// run server
		server = new CServer();
		server->setConfig(s_config);
		server->run();

		// clean up
		delete server;
		CNetwork::cleanup();
		CLog::setLock(NULL);
		s_logMutex = NULL;
	}
	catch (...) {
		delete server;
		CNetwork::cleanup();
		CLog::setLock(NULL);
		s_logMutex = NULL;
		throw;
	}
}


//
// command line parsing
//

static void				bye()
{
	log((CLOG_PRINT "Try `%s --help' for more information.", pname));
	exit(1);
}

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
	log((CLOG_PRINT
"Usage: %s"
" [--config <pathname>]"
" [--debug <level>]"
" [--daemon|--no-daemon]"
" [--restart|--no-restart]\n"
"Start the synergy mouse/keyboard sharing server.\n"
"\n"
"  -c, --config <pathname>  use the named configuration file instead\n"
"                           where ~ represents the user's home directory.\n"
"  -d, --debug <level>      filter out log messages with priorty below level.\n"
"                           level may be: FATAL, ERROR, WARNING, NOTE, INFO,\n"
"                           DEBUG, DEBUG1, DEBUG2.\n"
"  -f, --no-daemon          run the server in the foreground.\n"
"      --daemon             run the server as a daemon.\n"
"  -1, --no-restart         do not try to restart the server if it fails for\n"
"                           some reason.\n"
"      --restart            restart the server automatically if it fails.\n"
"  -h, --help               display this help and exit.\n"
"      --version            display version information and exit.\n"
"\n"
"By default, the server is a restartable daemon.  If no configuration file\n"
"pathname is provided then the first of the following to load sets the\n"
"configuration:\n"
"  " CONFIG_USER_DIR CONFIG_NAME "\n"
"  " CONFIG_SYS_DIR CONFIG_NAME "\n"
"If no configuration file can be loaded then the configuration uses its\n"
"defaults with just the server screen.\n"
"\n"
"Where log messages go depends on the platform and whether or not the\n"
"server is running as a daemon.",
								pname));

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
			log((CLOG_PRINT "%s: cannot read configuration '%s'",
								pname, pathname));
			exit(1);
		}
		else {
			log((CLOG_DEBUG "cannot read configuration \"%s\"", pathname));
		}
	}
	return false;
}

static bool				isArg(int argi,
								int argc, char** argv,
								const char* name1,
								const char* name2,
								int minRequiredParameters = 0)
{
	if ((name1 != NULL && strcmp(argv[argi], name1) == 0) ||
		(name2 != NULL && strcmp(argv[argi], name2) == 0)) {
		// match.  check args left.
		if (argi + minRequiredParameters >= argc) {
			log((CLOG_PRINT "%s: missing arguments for `%s'",
								pname, argv[argi]));
			bye();
		}
		return true;
	}

	// no match
	return false;
}

static void				parse(int argc, char** argv)
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

		else if (isArg(i, argc, argv, "-h", "--help")) {
			help();
			exit(1);
		}

		else if (isArg(i, argc, argv, NULL, "--version")) {
			version();
			exit(1);
		}

		else if (isArg(i, argc, argv, "--", NULL)) {
			// remaining arguments are not options
			++i;
			break;
		}

		else if (argv[i][0] == '-') {
			log((CLOG_PRINT "%s: unrecognized option `%s'", pname, argv[i]));
			bye();
		}

		else {
			// this and remaining arguments are not options
			break;
		}
	}

	// no non-option arguments are allowed
	if (i != argc) {
		log((CLOG_PRINT "%s: unrecognized option `%s'", pname, argv[i]));
		bye();
	}

	// set log filter
	if (!CLog::setFilter(s_logFilter)) {
		log((CLOG_PRINT "%s: unrecognized log level `%s'", pname, s_logFilter));
		bye();
	}

	// load the config file, if any
	if (s_configFile != NULL) {
		// require the user specified file to load correctly
		loadConfig(s_configFile, true);
	}
}


//
// platform dependent entry points
//

#if defined(CONFIG_PLATFORM_WIN32)

#include "CMSWindowsScreen.h"
#include <string.h>

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int)
{
	CPlatform platform;

	// save instance
	CMSWindowsScreen::init(instance);

	// get program name
	pname = platform.getBasename(argv[0]);

// FIXME -- direct CLog to MessageBox

	parse(__argc, __argv);

// FIXME -- undirect CLog from MessageBox
// FIXME -- if daemon then use win32 event log (however that's done),
// otherwise do what?  want to use console window for debugging but
// not otherwise.

	// load the configuration file if we haven't already
	if (s_configFile == NULL) {
// FIXME
	}

// FIXME
	if (__argc != 1) {
		CString msg = "no arguments allowed.  exiting.";
		MessageBox(NULL, msg.c_str(), "error", MB_OK | MB_ICONERROR);
		return 1;
	}

	try {
		realMain();
		return 0;
	}
	catch (XBase& e) {
		log((CLOG_CRIT "failed: %s", e.what()));
		CString msg = "failed: ";
		msg += e.what();
		MessageBox(NULL, msg.c_str(), "error", MB_OK | MB_ICONERROR);
		return 1;
	}
	catch (XThread&) {
		// terminated
		return 1;
	}
}

#elif defined(CONFIG_PLATFORM_UNIX)

#include <unistd.h>		// fork()
#include <sys/types.h>	// wait()
#include <sys/wait.h>	// wait()

int main(int argc, char** argv)
{
	CPlatform platform;

	// get program name
	pname = platform.getBasename(argv[0]);

	// parse command line
	parse(argc, argv);

	// load the configuration file if we haven't already
	if (s_configFile == NULL) {
		// get the user's home directory.  use the effective user id
		// so a user can't get a setuid root program to load his file.
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

	// daemonize if requested
	if (s_daemon) {
		if (!platform.daemonize("synergyd")) {
			log((CLOG_CRIT "failed to daemonize"));
			return 16;
		}
	}

	// run the server.  if running as a daemon then run it in a child
	// process and restart it as necessary.  we have to do this in case
	// the X server restarts because our process cannot recover from
	// that.
	for (;;) {
		// don't fork if not restartable
		switch (s_restartable ? fork() : 0) {
		default: {
			// parent process.  wait for child to exit.
			int status;
			if (wait(&status) == -1) {
				// wait failed.  this is unexpected so bail.
				log((CLOG_CRIT "wait() failed"));
				return 16;
			}

			// what happened?  if the child exited normally with a
			// status less than 16 then the child was deliberately
			// terminated so we also terminate.  otherwise, we
			// loop.
			if (WIFEXITED(status) && WEXITSTATUS(status) < 16) {
				return 0;
			}
			break;
		}

		case -1:
			// fork() failed.  log the error and proceed as a child
			log((CLOG_WARN "fork() failed;  cannot automatically restart on error"));
			// fall through

		case 0:
			// child process
			try {
				realMain();
				return 0;
			}
			catch (XBase& e) {
				log((CLOG_CRIT "failed: %s", e.what()));
				return 16;
			}
			catch (XThread&) {
				// terminated
				return 1;
			}
		}
	}
}

#else

#error no main() for platform

#endif
