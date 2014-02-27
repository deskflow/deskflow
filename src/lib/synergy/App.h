/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012 Bolton Software Ltd.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "common.h"
#include "CString.h"
#include "IApp.h"
#include "CIpcClient.h"

#if SYSAPI_WIN32
#include "CAppUtilWindows.h"
#elif SYSAPI_UNIX
#include "CAppUtilUnix.h"
#endif

class IArchTaskBarReceiver;
class CBufferedLogOutputter;
class ILogOutputter;
class CFileLogOutputter;
class CScreen;
class IEventQueue;
class CSocketMultiplexer;

typedef IArchTaskBarReceiver* (*CreateTaskBarReceiverFunc)(const CBufferedLogOutputter*, IEventQueue* events);

class CApp : public IApp {
public:
	CApp(IEventQueue* events, CreateTaskBarReceiverFunc createTaskBarReceiver, CArgsBase* args);
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

	// If --log was specified in args, then add a file logger.
	void setupFileLogging();

	// If messages will be hidden (to improve performance), warn user.
	void loggingFilterWarning();

	// Parses args, sets up file logging, and loads the config.
	void initApp(int argc, const char** argv);

	// HACK: accept non-const, but make it const anyway
	void initApp(int argc, char** argv) { initApp(argc, (const char**)argv); }

	ARCH_APP_UTIL& appUtil() { return m_appUtil; }

	virtual IArchTaskBarReceiver* taskBarReceiver() const  { return m_taskBarReceiver; }

	virtual void setByeFunc(void(*bye)(int)) { m_bye = bye; }
	virtual void bye(int error) { m_bye(error); }
	
	virtual IEventQueue* getEvents() const { return m_events; }

	void				setSocketMultiplexer(CSocketMultiplexer* sm) { m_socketMultiplexer = sm; }
	CSocketMultiplexer*	getSocketMultiplexer() const { return m_socketMultiplexer; }

private:
	void				handleIpcMessage(const CEvent&, void*);

protected:
	virtual void parseArgs(int argc, const char* const* argv, int &i);
	virtual bool parseArg(const int& argc, const char* const* argv, int& i);
	void				initIpcClient();
	void				cleanupIpcClient();
	void				runEventsLoop(void*);

	IArchTaskBarReceiver* m_taskBarReceiver;
	bool m_suspended;
	IEventQueue*		m_events;

private:
	CArgsBase* m_args;
	static CApp* s_instance;
	CFileLogOutputter* m_fileLog;
	CreateTaskBarReceiverFunc m_createTaskBarReceiver;
	ARCH_APP_UTIL m_appUtil;
	CIpcClient*			m_ipcClient;
	CSocketMultiplexer*	m_socketMultiplexer;
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
	"  -l  --log <file>         write log messages to file.\n" \
	"      --no-tray            disable the system tray icon.\n"

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

// windows args
#  define HELP_SYS_ARGS \
	" [--service <action>] [--relaunch] [--exit-pause]"
#  define HELP_SYS_INFO \
	"      --service <action>   manage the windows service, valid options are:\n" \
	"                             install/uninstall/start/stop\n" \
	"      --relaunch           persistently relaunches process in current user \n" \
	"                             session (useful for vista and upward).\n" \
	"      --exit-pause         wait for key press on exit, can be useful for\n" \
	"                             reading error messages that occur on exit.\n"
#endif
