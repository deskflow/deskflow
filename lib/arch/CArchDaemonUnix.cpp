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

#include "CArchDaemonUnix.h"
#include "XArchUnix.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <cstdlib>

//
// CArchDaemonUnix
//

CArchDaemonUnix::CArchDaemonUnix()
{
	// do nothing
}

CArchDaemonUnix::~CArchDaemonUnix()
{
	// do nothing
}

int
CArchDaemonUnix::daemonize(const char* name, DaemonFunc func)
{
	int dummy;
	
	// fork so shell thinks we're done and so we're not a process
	// group leader
	switch (fork()) {
	case -1:
		// failed
		throw XArchDaemonFailed(new XArchEvalUnix(errno));

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
	dummy = chdir("/");

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
	dummy = dup(1);

	// invoke function
	return func(1, &name);
}
