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

#include "server/ClientProxy1_2.h"

#include "synergy/ProtocolUtil.h"
#include "base/Log.h"

//
// ClientProxy1_1
//

ClientProxy1_2::ClientProxy1_2 (const String& name, synergy::IStream* stream,
                                IEventQueue* events)
    : ClientProxy1_1 (name, stream, events) {
    // do nothing
}

ClientProxy1_2::~ClientProxy1_2 () {
    // do nothing
}

void
ClientProxy1_2::mouseRelativeMove (SInt32 xRel, SInt32 yRel) {
    LOG ((CLOG_DEBUG2 "send mouse relative move to \"%s\" %d,%d",
          getName ().c_str (),
          xRel,
          yRel));
    ProtocolUtil::writef (getStream (), kMsgDMouseRelMove, xRel, yRel);
}
