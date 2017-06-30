/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2003 Chris Schoeneman
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

#include "XWindowsClientTaskBarReceiver.h"
#include "arch/Arch.h"

//
// CXWindowsClientTaskBarReceiver
//

CXWindowsClientTaskBarReceiver::CXWindowsClientTaskBarReceiver (
    const BufferedLogOutputter*, IEventQueue* events)
    : ClientTaskBarReceiver (events) {
    // add ourself to the task bar
    ARCH->addReceiver (this);
}

CXWindowsClientTaskBarReceiver::~CXWindowsClientTaskBarReceiver () {
    ARCH->removeReceiver (this);
}

void
CXWindowsClientTaskBarReceiver::showStatus () {
    // do nothing
}

void
CXWindowsClientTaskBarReceiver::runMenu (int, int) {
    // do nothing
}

void
CXWindowsClientTaskBarReceiver::primaryAction () {
    // do nothing
}

const IArchTaskBarReceiver::Icon
CXWindowsClientTaskBarReceiver::getIcon () const {
    return NULL;
}

IArchTaskBarReceiver*
createTaskBarReceiver (const BufferedLogOutputter* logBuffer,
                       IEventQueue* events) {
    return new CXWindowsClientTaskBarReceiver (logBuffer, events);
}
