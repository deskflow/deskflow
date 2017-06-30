/*
 * synergy -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#include "OSXClientTaskBarReceiver.h"
#include "arch/Arch.h"

//
// OSXClientTaskBarReceiver
//

OSXClientTaskBarReceiver::OSXClientTaskBarReceiver (const BufferedLogOutputter*,
                                                    IEventQueue* events)
    : ClientTaskBarReceiver (events) {
    // add ourself to the task bar
    ARCH->addReceiver (this);
}

OSXClientTaskBarReceiver::~OSXClientTaskBarReceiver () {
    ARCH->removeReceiver (this);
}

void
OSXClientTaskBarReceiver::showStatus () {
    // do nothing
}

void
OSXClientTaskBarReceiver::runMenu (int, int) {
    // do nothing
}

void
OSXClientTaskBarReceiver::primaryAction () {
    // do nothing
}

const IArchTaskBarReceiver::Icon
OSXClientTaskBarReceiver::getIcon () const {
    return NULL;
}

IArchTaskBarReceiver*
createTaskBarReceiver (const BufferedLogOutputter* logBuffer,
                       IEventQueue* events) {
    return new OSXClientTaskBarReceiver (logBuffer, events);
}
