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

#include "CUnixPlatform.h"
#include "CLog.h"
#include <cstring>
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

bool
CUnixPlatform::installDaemon(const char*, const char*, const char*, const char*)
{
	// daemons don't require special installation
	return true;
}

CUnixPlatform::EResult
CUnixPlatform::uninstallDaemon(const char*)
{
	// daemons don't require special installation
	return kSuccess;
}

int
CUnixPlatform::daemonize(const char* name, DaemonFunc func)
{
#if HAVE_WORKING_FORK
	// fork so shell thinks we're done and so we're not a process
	// group leader
	switch (fork()) {
	case -1:
		// failed
		return -1;

	case 0:
		// child
		break;

	default:
		// parent exits
		exit(0);
	}
#endif

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
	installDaemonLogger(name);

	// invoke function
	return func(this, 1, &name);
}

void
CUnixPlatform::installDaemonLogger(const char* name)
{
	openlog(name, 0, LOG_DAEMON);
	CLog::setOutputter(&CUnixPlatform::deamonLogger);
}

const char*
CUnixPlatform::getBasename(const char* pathname) const
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

CString
CUnixPlatform::getUserDirectory() const
{
#if HAVE_GETPWUID_R
	struct passwd pwent;
	struct passwd* pwentp;
	long size = sysconf(_SC_GETPW_R_SIZE_MAX);
	char* buffer = new char[size];
	getpwuid_r(getuid(), &pwent, buffer, size, &pwentp);
	delete[] buffer;
#else
	struct passwd* pwentp = getpwuid(getuid());
#endif
	if (pwentp != NULL && pwentp->pw_dir != NULL) {
		return pwentp->pw_dir;
	}
	else {
		return CString();
	}
}

CString
CUnixPlatform::getSystemDirectory() const
{
	return "/etc";
}

CString
CUnixPlatform::addPathComponent(const CString& prefix,
				const CString& suffix) const
{
	CString path;
	path.reserve(prefix.size() + 1 + suffix.size());
	path += prefix;
	if (path.size() == 0 || path[path.size() - 1] != '/') {
		path += '/';
	}
	path += suffix;
	return path;
}

bool
CUnixPlatform::deamonLogger(int priority, const char* msg)
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
	return true;
}
