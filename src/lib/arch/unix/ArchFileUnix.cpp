/*
 * barrier -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
 * 
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 * 
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "arch/unix/ArchFileUnix.h"
#include "common/DataDirectories.h"

#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <cstring>

//
// ArchFileUnix
//

const char*
ArchFileUnix::getBasename(const char* pathname)
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
ArchFileUnix::getSystemDirectory()
{
    return "/etc";
}

std::string
ArchFileUnix::concatPath(const std::string& prefix,
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
