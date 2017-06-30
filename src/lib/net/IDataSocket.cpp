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

#include "net/IDataSocket.h"
#include "base/EventQueue.h"

//
// IDataSocket
//

void
IDataSocket::close () {
    // this is here to work around a VC++6 bug.  see the header file.
    assert (0 && "bad call");
}

void*
IDataSocket::getEventTarget () const {
    // this is here to work around a VC++6 bug.  see the header file.
    assert (0 && "bad call");
    return NULL;
}
