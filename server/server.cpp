#include "CServer.h"
#include "CConfig.h"
#include "CLog.h"
#include "CMutex.h"
#include "CNetwork.h"
#include "CThread.h"
#include "XThread.h"
#include "ProtocolTypes.h"
#include "stdfstream.h"
#include <assert.h>

static const char* s_copyright     = "Copyright (C) 2002 Chris Schoeneman";
static const SInt32 s_majorVersion = 0;
static const SInt32 s_minorVersion = 5;
static const char s_releaseVersion = ' ';

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
"%s %d.%d%c protocol version %d.%d\n"
"%s",
								pname,
								s_majorVersion,
								s_minorVersion,
								s_releaseVersion,
								kMajorVersion,
								kMinorVersion,
								s_copyright));
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
	CMSWindowsScreen::init(instance);

	// get program name
	pname = strrchr(argv[0], '/');
	if (pname == NULL) {
		pname = argv[0];
	}
	else {
		++pname;
	}
	const char* pname2 = strrchr(argv[0], '\\');
	if (pname2 != NULL && pname2 > pname) {
		pname = pname2 + 1;
	}

// FIXME -- direct CLog to MessageBox

	parse(__argc, __argv);

// FIXME -- undirect CLog from MessageBox
// FIXME -- if daemon then use win32 event log (however that's done),
// otherwise do what?  want to use console window for debugging but
// not otherwise.

	// load the configuration file if we haven't already
	if (s_configFile == NULL) {
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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

static const char* s_configFileDefault = CONFIG_SYS_DIR CONFIG_NAME;

static void				daemonize()
{
	// fork so shell thinks we're done and so we're not a process
	// group leader
	switch (fork()) {
	case -1:
		// failed
		log((CLOG_PRINT "failed to daemonize"));
		exit(1);

	case 0:
		// child
		break;

	default:
		// parent exits
		exit(0);
	}

	// become leader of a new session
	setsid();

	// chdir to root so we don't keep mounted filesystems points busy
	chdir("/");

	// mask off permissions for any but owner
	umask(077);

	// close open files.  we only expect stdin, stdout, stderr to be open.
	close(0);
	close(1);
	close(2);

	// attach file descriptors 0, 1, 2 to /dev/null so inadvertent use
	// of standard I/O safely goes in the bit bucket.
	open("/dev/null", O_RDWR);
	dup(0);
	dup(0);
}

static void				syslogOutputter(int priority, const char* msg)
{
	// convert priority
	switch (priority) {
	case CLog::kFATAL:
	case CLog::kERROR:
		priority = LOG_ERR;
		break;

	case CLog::kWARNING:
		priority = LOG_WARNING;
		break;

	case CLog::kNOTE:
		priority = LOG_NOTICE;
		break;

	case CLog::kINFO:
		priority = LOG_INFO;
		break;

	default:
		priority = LOG_DEBUG;
		break;
	}

	// log it
	syslog(priority, "%s", msg);
}

int main(int argc, char** argv)
{
	// get program name
	pname = strrchr(argv[0], '/');
	if (pname == NULL) {
		pname = argv[0];
	}
	else {
		++pname;
	}

	// parse command line
	parse(argc, argv);

	// load the configuration file if we haven't already
	if (s_configFile == NULL) {
		// get the user's home directory.  use the effective user id
		// so a user can't get a setuid root program to load his file.
		bool loaded = false;
		struct passwd* pwent = getpwuid(geteuid());
		if (pwent != NULL && pwent->pw_dir != NULL) {
			// construct path if it isn't too long
			if (strlen(pwent->pw_dir) + strlen(CONFIG_NAME) + 2 <= PATH_MAX) {
				char path[PATH_MAX];
				strcpy(path, pwent->pw_dir);
				strcat(path, "/");
				strcat(path, CONFIG_NAME);

				// now try loading the user's configuration
				loaded = loadConfig(path, false);
			}
		}
		if (!loaded) {
			// try the system-wide config file
			loadConfig(s_configFileDefault, false);
		}
	}

	// daemonize if requested
	if (s_daemon) {
		daemonize();

		// send log to syslog
		openlog("synergy", 0, LOG_DAEMON);
		CLog::setOutputter(&syslogOutputter);
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
				fprintf(stderr, "failed: %s\n", e.what());
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
