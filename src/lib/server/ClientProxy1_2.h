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

#pragma once

#include "server/ClientProxy1_1.h"

class IEventQueue;

//! Proxy for client implementing protocol version 1.2
class ClientProxy1_2 : public ClientProxy1_1 {
public:
    ClientProxy1_2 (const String& name, synergy::IStream* adoptedStream,
                    IEventQueue* events);
    ~ClientProxy1_2 ();

    // IClient overrides
    virtual void mouseRelativeMove (SInt32 xRel, SInt32 yRel);
};
