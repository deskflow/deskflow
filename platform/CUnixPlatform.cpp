#include "CUnixPlatform.h"
#include "CLog.h"
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>

//
// CUnixPlatform
//

CUnixPlatform::CUnixPlatform()
{
	// do nothing
}

CUnixPlatform::~CUnixPlatform()
{
	// do nothing
}

bool					CUnixPlatform::installDaemon(/* FIXME */)
{
	// daemons don't require special installation
	return true;
}

bool					CUnixPlatform::uninstallDaemon(/* FIXME */)
{
	// daemons don't require special installation
	return true;
}

bool					CUnixPlatform::daemonize(const char* name)
{
	// fork so shell thinks we're done and so we're not a process
	// group leader
	switch (fork()) {
	case -1:
		// failed
		return false;

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
	open("/dev/null", O_RDONLY);
	open("/dev/null", O_RDWR);
	dup(1);

	// hook up logger
	setDaemonLogger(name);

	return true;
}

const char*				CUnixPlatform::getBasename(const char* pathname) const
{
	if (pathname == NULL) {
		return NULL;
	}

	const char* basename = strrchr(pathname, '/');
	if (basename != NULL) {
		return basename + 1;
	}
	else {
		return pathname;
	}
}

CString					CUnixPlatform::getUserDirectory() const
{
	// FIXME -- use geteuid?  shouldn't run this setuid anyway.
	struct passwd* pwent = getpwuid(getuid());
	if (pwent != NULL && pwent->pw_dir != NULL) {
		return pwent->pw_dir;
	}
	else {
		return CString();
	}
}

CString					CUnixPlatform::getSystemDirectory() const
{
	return "/etc";
}

CString					CUnixPlatform::addPathComponent(
								const CString& prefix,
								const CString& suffix) const
{
	CString path;
	path.reserve(prefix.size() + 1 + suffix.size());
	path += prefix;
	path += '/';
	path += suffix;
	return path;
}

void					CUnixPlatform::setDaemonLogger(const char* name)
{
	openlog(name, 0, LOG_DAEMON);
	CLog::setOutputter(&CUnixPlatform::deamonLogger);
}

void					CUnixPlatform::deamonLogger(
								int priority, const char* msg)
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
