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

#pragma once

#include "common.h"
#include "CString.h"

class IArchTaskBarReceiver;
class CBufferedLogOutputter;
class ILogOutputter;
class CFileLogOutputter;
class CScreen;

typedef IArchTaskBarReceiver* (*CreateTaskBarReceiverFunc)(const CBufferedLogOutputter*);
typedef int (*StartupFunc)(int, char**);

class CApp {
public:
	class CArgsBase {
	public:
		CArgsBase();
		virtual ~CArgsBase();
		bool m_daemon;
		bool m_backend;
		bool m_restartable;
		bool m_noHooks;
		const char* m_pname;
		const char* m_logFilter;
		const char*	m_logFile;
		const char*	m_display;
		CString m_name;
		bool m_disableTray;
#if SYSAPI_WIN32
		bool m_relaunchMode;
		bool m_debugServiceWait;
		bool m_pauseOnExit;
#endif
#if WINAPI_XWINDOWS
		bool m_disableXInitThreads;
#endif
	};

	CApp(CreateTaskBarReceiverFunc createTaskBarReceiver, CArgsBase* args);
	virtual ~CApp();

	// Returns args that are common between server and client.
	CArgsBase& argsBase() const { return *m_args; }

	// Prints the current compiled version.
	virtual void version();

	// Prints help specific to client or server.
	virtual void help() = 0;

	// Parse command line arguments.
	virtual void parseArgs(int argc, const char* const* argv) = 0;
	
	int run(int argc, char** argv);

	int daemonMainLoop(int, const char**);

	virtual void loadConfig() = 0;
	virtual bool loadConfig(const CString& pathname) = 0;
	virtual int mainLoop() = 0;
	virtual int foregroundStartup(int argc, char** argv) = 0;
	virtual int standardStartup(int argc, char** argv) = 0;
	virtual int runInner(int argc, char** argv, ILogOutputter* outputter, StartupFunc startup) = 0;

	// Name of the daemon (used for Unix and Windows).
	virtual const char* daemonName() const = 0;

	// A description of the daemon (used only on Windows).
	virtual const char* daemonInfo() const = 0;

	// Function pointer for function to exit immediately.
	// TODO: this is old C code - use inheritance to normalize
	void (*m_bye)(int);

	// Returns true if argv[argi] is equal to name1 or name2.
	bool isArg(int argi, int argc, const char* const* argv,
		const char* name1, const char* name2,
		int minRequiredParameters = 0);

	static CApp& instance() { assert(s_instance != nullptr); return *s_instance; }

	bool m_suspended;
	IArchTaskBarReceiver* m_taskBarReceiver;

	// If --log was specified in args, then add a file logger.
	void setupFileLogging();

	// If messages will be hidden (to improve performance), warn user.
	void loggingFilterWarning();

	// Parses args, sets up file logging, and loads the config.
	void initApp(int argc, const char** argv);

	// HACK: accept non-const, but make it const anyway
	void initApp(int argc, char** argv) { initApp(argc, (const char**)argv); }

	// Start the server or client.
	virtual void startNode() = 0;

	virtual CScreen* createScreen() = 0;

protected:
	virtual void parseArgs(int argc, const char* const* argv, int &i);
	virtual bool parseArg(const int& argc, const char* const* argv, int& i);

private:
	CArgsBase* m_args;
	static CApp* s_instance;
	CFileLogOutputter* m_fileLog;
	CreateTaskBarReceiverFunc m_createTaskBarReceiver;
};

#define BYE "\nTry `%s --help' for more information."

#if WINAPI_MSWINDOWS
#define DAEMON_RUNNING(running_) CArchMiscWindows::daemonRunning(running_)
#else
#define DAEMON_RUNNING(running_)
#endif

#define HELP_COMMON_INFO_1 \
	"  -d, --debug <level>      filter out log messages with priority below level.\n" \
	"                             level may be: FATAL, ERROR, WARNING, NOTE, INFO,\n" \
	"                             DEBUG, DEBUGn (1-5).\n" \
	"  -n, --name <screen-name> use screen-name instead the hostname to identify\n" \
	"                             this screen in the configuration.\n" \
	"  -1, --no-restart         do not try to restart on failure.\n" \
	"*     --restart            restart the server automatically if it fails.\n" \
	"  -l  --log <file>         write log messages to file.\n"

#define HELP_COMMON_INFO_2 \
	"  -h, --help               display this help and exit.\n" \
	"      --version            display version information and exit.\n"

#define HELP_COMMON_ARGS \
	" [--name <screen-name>]" \
	" [--restart|--no-restart]" \
	" [--debug <level>]"

// system args (windows/unix)
#if SYSAPI_UNIX

// unix daemon mode args
#  define HELP_SYS_ARGS \
	" [--daemon|--no-daemon]"
#  define HELP_SYS_INFO \
	"  -f, --no-daemon          run in the foreground.\n"	\
	"*     --daemon             run as a daemon.\n"

#elif SYSAPI_WIN32

// windows service management args
#  define HELP_SYS_ARGS \
	" [--service <action>]"
#  define HELP_SYS_INFO \
	"      --service <action>   manage the windows service, valid options are:\n" \
	"                             install/uninstall/start/stop\n" \
	"      --relaunch           persistently relaunches process in current user \n" \
	"                             session (useful for vista and upward).\n" \
	"      --exit-pause         wait for key press on exit, can be useful for\n" \
	"                             reading error messages that occur on exit.\n" \
	"      --no-tray            disable the system tray icon.\n"

#endif
