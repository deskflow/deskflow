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

#include "CArchFileUnix.h"
#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <cstring>

//
// CArchFileUnix
//

CArchFileUnix::CArchFileUnix()
{
	// do nothing
}

CArchFileUnix::~CArchFileUnix()
{
	// do nothing
}

const char*
CArchFileUnix::getBasename(const char* pathname)
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

std::string
CArchFileUnix::getUserDirectory()
{
	char* buffer = NULL;
	std::string dir;
#if HAVE_GETPWUID_R
	struct passwd pwent;
	struct passwd* pwentp;
#if defined(_SC_GETPW_R_SIZE_MAX)
	long size = sysconf(_SC_GETPW_R_SIZE_MAX);
	if (size == -1) {
		size = BUFSIZ;
	}
#else
	long size = BUFSIZ;
#endif
	buffer = new char[size];
	getpwuid_r(getuid(), &pwent, buffer, size, &pwentp);
#else
	struct passwd* pwentp = getpwuid(getuid());
#endif
	if (pwentp != NULL && pwentp->pw_dir != NULL) {
		dir = pwentp->pw_dir;
	}
	delete[] buffer;
	return dir;
}

std::string
CArchFileUnix::getSystemDirectory()
{
	return "/etc";
}

std::string
CArchFileUnix::concatPath(const std::string& prefix,
				const std::string& suffix)
{
	std::string path;
	path.reserve(prefix.size() + 1 + suffix.size());
	path += prefix;
	if (path.size() == 0 || path[path.size() - 1] != '/') {
		path += '/';
	}
	path += suffix;
	return path;
}
