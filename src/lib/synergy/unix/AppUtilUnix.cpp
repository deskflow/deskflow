/*
 * synergy -- mouse and keyboard sharing utility
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

#include "synergy/unix/AppUtilUnix.h"
#include "synergy/ArgsBase.h"

AppUtilUnix::AppUtilUnix (IEventQueue* events) {
}

AppUtilUnix::~AppUtilUnix () {
}

int
standardStartupStatic (int argc, char** argv) {
    return AppUtil::instance ().app ().standardStartup (argc, argv);
}

int
AppUtilUnix::run (int argc, char** argv) {
    return app ().runInner (argc, argv, NULL, &standardStartupStatic);
}

void
AppUtilUnix::startNode () {
    app ().startNode ();
}
